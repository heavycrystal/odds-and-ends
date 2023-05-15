import argparse
import asyncio
import ctypes
import logging
import platform
import random
import signal
import time
from collections import namedtuple
from dataclasses import dataclass
from enum import IntEnum
from pprint import pformat
from typing import Any, Callable, NewType, Optional, TypeAlias, final

OBJECTID_GLOBAL_RANDBYTES: bytes = random.randbytes(5)


def _fixedint_classgen(name: str, size: int, signed: bool) -> type:
    from_bytes: Callable[[bytes], int] = staticmethod(
        lambda source: int.from_bytes(source[:size], "little")
    )
    to_bytes: Callable[[int], bytes] = lambda self: int.to_bytes(
        self, size, "little", signed=signed
    )

    return final(
        type(
            name,
            (int,),
            {
                "from_bytes": from_bytes,
                "to_bytes": to_bytes,
                "__len__": lambda _: size,
            },
        )
    )


# Dynamic languages for the win!
uint8: TypeAlias = _fixedint_classgen(name="uint8", size=1, signed=False)
int32: TypeAlias = _fixedint_classgen(name="int32", size=4, signed=True)
int64: TypeAlias = _fixedint_classgen(name="int64", size=8, signed=True)
uint64: TypeAlias = _fixedint_classgen(name="uint64", size=8, signed=False)


@final
class BsonElementTypes(IntEnum):
    DOUBLE = 0x01
    STRING = 0x02
    DOCUMENT = 0x03
    ARRAY = 0x04
    BINARY = 0x05
    OBJECT_ID = 0x07
    BOOLEAN = 0x08
    NULL = 0x0A
    INT32 = 0x10
    UINT64 = 0x11
    INT64 = 0x12


@final
class MessageOpcodes(IntEnum):
    OP_REPLY = 1
    OP_QUERY = 2004
    OP_COMPRESSED = 2012
    OP_MSG = 2013


@final
class CompressorIds(IntEnum):
    NOOP = 0
    ZLIB = 2


def ctypes_encode(x) -> bytes: return x._objects.tobytes()


BsonBinaryType = namedtuple("BsonBinaryType", ["subtype", "data"])
BsonBaseType = (
        ctypes.c_double
        | str
        | BsonBinaryType
        | ctypes.c_bool
        | int32
        | int64
        | uint64
)
BsonDocumentDict = NewType("BsonDocumentDict", dict[str, tuple[BsonElementTypes, Any]])
BsonArrayList = NewType("BsonArrayList", list[tuple[BsonElementTypes, Any]])


@final
@dataclass(frozen=True, kw_only=True)
class MessageHeader:
    message_length: int32
    request_id: int32
    response_to: int32
    op_code: int32

    @staticmethod
    def from_bytearray(source: bytearray):
        message_length: int32 = int32.from_bytes(source)
        request_id: int32 = int32.from_bytes(source[4:8])
        response_to: int32 = int32.from_bytes(source[8:12])
        op_code: int32 = int32.from_bytes(source[12:16])

        return MessageHeader(
            message_length=message_length,
            request_id=request_id,
            response_to=response_to,
            op_code=op_code,
        )

    def to_bytes(self) -> bytes:
        return (
                ctypes_encode(self.message_length)
                + ctypes_encode(self.request_id)
                + ctypes_encode(self.response_to)
                + ctypes_encode(self.op_code)
        )

    def __len__(self) -> int:
        return 16


@final
@dataclass(frozen=True, kw_only=True)
class BsonObjectId:
    timestamp: bytes
    random_value: bytes
    counter: bytes

    @staticmethod
    def new():
        global OBJECTID_GLOBAL_RANDBYTES

        return BsonObjectId(
            timestamp=int(time.time()).to_bytes(4, "big"),
            random_value=OBJECTID_GLOBAL_RANDBYTES,
            counter=random.randbytes(3),
        )

    def __len__(self):
        return 12


@final
@dataclass(frozen=True, kw_only=True)
class BsonDocument:
    document_length: int32
    is_array: bool
    elements: BsonDocumentDict | BsonArrayList

    @staticmethod
    def string_decode(source: bytearray) -> str:
        string_length: int32 = int32.from_bytes(source[:4])
        return source[4: (string_length + 3)].decode()

    @staticmethod
    def binary_decode(source: bytearray) -> BsonBinaryType:
        binary_length: int32 = int32.from_bytes(source[:4])
        subtype: int = ctypes.c_uint8.from_buffer(source[4:5]).value

        return BsonBinaryType(
            subtype,
            bytes(source[5: (binary_length + 5)]),
        )

    @staticmethod
    def string_encode(source: str) -> bytes:
        string_length: int32 = int32(len(source) + 1)
        return string_length.to_bytes() + source.encode() + b"\0"

    @staticmethod
    def cstring_encode(source: str) -> bytes:
        return source.encode() + b"\0"

    @classmethod
    def binary_encode(cls, source: tuple) -> bytes:
        binary_length: int32 = int32(len(source[1]) + 1)
        return binary_length.to_bytes() + ctypes_encode(source[0]) + source[1]

    @classmethod
    def from_bytearray(cls, source: bytearray, is_array: bool):
        document_length: int32 = int32.from_bytes(source)
        elements: BsonDocumentDict | BsonArrayList = (
            BsonArrayList([]) if is_array else BsonDocumentDict({})
        )
        current_offset: int32 = int32(4)

        # in here because dataclasses don't like mutable collections
        ELEMENT_ENCODE_TABLE: dict[
            BsonElementTypes, tuple[Callable[[bytearray], Any], Callable[[Any], int]]
        ] = {
            BsonElementTypes.DOUBLE: (
                lambda x: ctypes.c_double.from_buffer(x[:8]),
                lambda _: 8,
            ),
            BsonElementTypes.STRING: (
                cls.string_decode,
                lambda x: len(x) + 5,
            ),  # 4 bytes size, 1 byte null terminator.
            BsonElementTypes.DOCUMENT: (
                lambda x: cls.from_bytearray(x, False),
                lambda x: x.document_length,
            ),
            BsonElementTypes.ARRAY: (
                lambda x: cls.from_bytearray(x, True),
                lambda x: x.document_length,
            ),
            BsonElementTypes.BINARY: (
                cls.binary_decode,
                lambda x: len(x.data.raw) + 5,  # 4 bytes size, 1 byte subtype
            ),
            BsonElementTypes.BOOLEAN: (
                lambda x: ctypes.c_bool.from_buffer(x[:1]),
                lambda _: 1,
            ),
            BsonElementTypes.NULL: (lambda _: None, lambda _: 0),
            BsonElementTypes.INT32: (
                lambda x: int32.from_bytes(x[:4]),
                len,
            ),
            BsonElementTypes.UINT64: (
                lambda x: ctypes.c_uint64.from_buffer(x[:8]),
                lambda _: 8,
            ),
            BsonElementTypes.INT64: (
                lambda x: int64.from_bytes(x[:8]),
                len,
            ),
        }

        while source[current_offset] != 0x00:
            element_type: int = source[current_offset]
            if element_type not in iter(BsonElementTypes):
                logging.warning(
                    f"Unsupported element type {hex(element_type)} encountered while decoding, aborting decode."
                )
                break
            current_offset += 1

            element_name = ctypes.c_char_p(
                bytes(source[current_offset:])
            ).value.decode()
            current_offset += len(element_name) + 1

            decode_functions = ELEMENT_ENCODE_TABLE[BsonElementTypes(element_type)]
            element_value: Any = decode_functions[0](source[current_offset:])
            current_offset += decode_functions[1](element_value)

            if is_array:
                elements.append((BsonElementTypes(element_type), element_value))
            else:
                elements[element_name] = (BsonElementTypes(element_type), element_value)

        return BsonDocument(
            document_length=document_length, is_array=is_array, elements=elements
        )

    def to_bytes(self) -> bytes:
        # in here because dataclasses don't like mutable collections
        ELEMENT_DECODE_TABLE: dict[
            BsonElementTypes, Callable[[Any], bytes | bytearray]
        ] = {
            BsonElementTypes.DOUBLE: ctypes_encode,
            BsonElementTypes.STRING: self.string_encode,
            BsonElementTypes.DOCUMENT: lambda x: x.to_bytearray(),
            BsonElementTypes.ARRAY: lambda x: x.to_bytearray(),
            BsonElementTypes.BINARY: self.binary_encode,
            BsonElementTypes.BOOLEAN: ctypes_encode,
            BsonElementTypes.NULL: lambda _: bytes(),
            BsonElementTypes.INT32: lambda x: x.to_bytes(),
            BsonElementTypes.UINT64: ctypes_encode,
            BsonElementTypes.INT64: lambda x: x.to_bytes(),
        }

        output = self.document_length.to_bytes()
        if self.is_array:
            for index, element in enumerate(self.elements):
                output += element[0].to_bytes(1, "little")
                output += self.cstring_encode(str(index))
                output += ELEMENT_DECODE_TABLE[element[0]](element[1])
        else:
            for element in self.elements.items():
                output += element[1][0].to_bytes(1, "little")
                output += self.cstring_encode(element[0])
                output += ELEMENT_DECODE_TABLE[element[1][0]](element[1][1])
        return bytes(output) + b"\0"

    def __len__(self) -> int:
        return self.document_length

    def __repr__(self) -> str:
        output: dict[str, Any] | list[Any] = {}

        if self.is_array:
            output = [
                element[1].value if getattr(element[1], "value", None) else element[1]
                for element in self.elements
            ]
        else:
            for element in self.elements.items():
                output[element[0]] = (
                    element[1][1].value
                    if getattr(element[1][1], "value", None)
                    else element[1][1]
                )

        return pformat(output)


@final
@dataclass(frozen=True, kw_only=True)
class OpQueryMessage:
    message_header: MessageHeader
    flags: int32
    full_collection_name: str
    number_to_skip: int32
    number_to_return: int32
    document: BsonDocument

    @staticmethod
    def from_bytearray(source: bytearray):
        msg_header: MessageHeader = MessageHeader.from_bytearray(source[:16])
        flags: int32 = int32.from_bytes(source[16:])
        full_collection_name: str = ctypes.c_char_p(bytes(source[20:])).value.decode()
        number_to_skip: int32 = int32.from_bytes(
            source[(20 + len(full_collection_name) + 1):]
        )
        number_to_return: int32 = int32.from_bytes(
            source[(24 + len(full_collection_name) + 1):]
        )
        total_header_length: int = (
                len(msg_header) + 4 + (len(full_collection_name) + 1) + 4 + 4
        )
        document: BsonDocument = BsonDocument.from_bytearray(
            source[total_header_length:], False
        )

        return OpQueryMessage(
            message_header=msg_header,
            flags=flags,
            full_collection_name=full_collection_name,
            number_to_skip=number_to_skip,
            number_to_return=number_to_return,
            document=document,
        )


@final
@dataclass(frozen=True, kw_only=True)
class OpReplyMessage:
    message_header: MessageHeader
    response_flags: int32
    cursor_id: int64
    starting_from: int32
    number_returned: int32
    documents: list[BsonDocument]

    @staticmethod
    def from_documents(request_id: int32, documents: list[BsonDocument]):
        documents_length: int32 = int32(sum([len(document) for document in documents]))
        message_header: MessageHeader = MessageHeader(
            message_length=int32(16 + 4 + 8 + 4 + 4 + documents_length),
            request_id=request_id,
            response_to=request_id,
            op_code=int32(MessageOpcodes.OP_REPLY),
        )
        response_flags: int32 = int32(0)
        cursor_id: int64 = int64(0)
        starting_from: int32 = int32(0)
        number_returned: int32 = int32(len(documents))

        return OpReplyMessage(
            message_header=message_header,
            response_flags=response_flags,
            cursor_id=cursor_id,
            starting_from=starting_from,
            number_returned=number_returned,
            documents=documents,
        )


@final
@dataclass(frozen=True, kw_only=True)
class OpMsgMessage:
    message_header: MessageHeader
    flag_bits: int
    sections: list[BsonDocument | list[BsonDocument]]
    checksum: Optional[int]


@final
@dataclass(frozen=True, kw_only=True)
class OpCompressedMessage:
    message_header: MessageHeader
    original_opcode: MessageOpcodes
    uncompressed_size: int32
    compressor_id: CompressorIds
    compressed_message: bytes

    @staticmethod
    def from_bytearray(source: bytearray):
        message_header: MessageHeader = MessageHeader.from_bytearray(source[:16])
        original_opcode: MessageOpcodes = MessageOpcodes(
            int32.from_bytes(source[16:20])
        )
        uncompressed_size: int32 = int32.from_bytes(source[20:24])
        compressor_id: CompressorIds = CompressorIds(
            ctypes.c_ubyte.from_buffer(source[24:25]).value
        )
        compressed_message: bytes = bytes(source[25:])

        return OpCompressedMessage(
            message_header=message_header,
            original_opcode=original_opcode,
            uncompressed_size=uncompressed_size,
            compressor_id=compressor_id,
            compressed_message=compressed_message,
        )


def setup_logging(level: str) -> None:
    logging.basicConfig(
        level=getattr(logging, level.upper()),
        format="%(asctime)s.%(msecs)d %(levelname)s: %(message)s",
        datefmt="%H:%M:%S",
    )


def parse_arguments() -> argparse.Namespace:
    argument_parser = argparse.ArgumentParser(prog="micro_mongo")
    argument_parser.add_argument(
        "--port",
        default=27017,
        type=lambda x: int(x) if 1 < int(x) < 65536 else 27017,
        metavar="port",
        help="Port number to run server on.",
    )
    argument_parser.add_argument(
        "--host", default="127.0.0.1", type=str, help="IP Address to bind server to."
    )
    argument_parser.add_argument(
        "--loglevel",
        default="info",
        type=str,
        choices=["debug", "info", "warning", "error", "critical"],
        help="Level at which server logs should be printed.",
    )
    return argument_parser.parse_args()


def display_startup_messages(parameters: argparse.Namespace) -> None:
    logging.info(f"micro_mongo server starting on port {parameters.port}")
    logging.info(
        f"Platform: {platform.platform()}, "
        f"{platform.python_implementation()} {platform.python_version()}"
    )


async def connection_handler(
        reader: asyncio.StreamReader, writer: asyncio.StreamWriter
) -> None:
    logging.info(f"Accepted connection from {writer.get_extra_info('peername')}")
    while True:
        data = await reader.read()
        if not data:
            break
        logging.info(f"Message length: {len(data)}")
        msg_header = MessageHeader.from_bytearray(bytearray(data))
        if msg_header.op_code == MessageOpcodes.OP_QUERY:
            logging.info(pformat(OpQueryMessage.from_bytearray(bytearray(data))))
        else:
            logging.warning(
                f"Received message with unknown opcode: {msg_header.op_code}"
            )
        writer.write(data)


def loop_exception_handler(_, context: dict):
    logging.error(f"Task error: {context['message']}")
    if context.get("exception", None):
        logging.error(f"Exception encountered was: {context['exception']}")


async def loop_shutdown(
        s: signal.Signals, el: asyncio.AbstractEventLoop
) -> None:
    logging.warning(f"Stopping due to {s.name}...")
    tasks = [task for task in asyncio.all_tasks() if task is not asyncio.current_task()]
    [task.cancel() for task in tasks]
    await asyncio.gather(*tasks, return_exceptions=True)

    logging.warning(f"Cancelled {len(tasks)} tasks.")
    el.stop()


if __name__ == "__main__":
    parameters = parse_arguments()
    setup_logging(parameters.loglevel)
    display_startup_messages(parameters)

    event_loop = asyncio.new_event_loop()
    signals = [signal.SIGINT, signal.SIGTERM, signal.SIGHUP]
    for sigs in signals:
        event_loop.add_signal_handler(
            sigs,
            lambda s=sigs: asyncio.create_task(loop_shutdown(s, event_loop)),
        )
    # event_loop.set_exception_handler(loop_exception_handler)

    try:
        server = asyncio.start_server(
            connection_handler, host=parameters.host, port=parameters.port
        )
        event_loop.run_until_complete(server)
        event_loop.run_forever()
    finally:
        event_loop.close()
        logging.info("Successfully shutdown micro_mongo server.")
