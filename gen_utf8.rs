use std::fs::File;
use std::io::Write;

fn main()
{
    let mut output_file_handle = File::options().write(true).create_new(true).open("utf-8.txt").unwrap();
    let mut buffer: Vec<u8> = (0..128).collect(); // one byte Unicode codepoints generated here.

    // two byte Unicode codepoints
    for i in 0x80..0x800
    {
        buffer.push(0b11000000 + (i >> 6) as u8);
        buffer.push(0b10000000 + (i & 0b00111111) as u8);
    }
    // three byte Unicode codepoints
    for i in 0x800..0x10000
    {
        // these are Unicode surrogates, have no meaning on their own. 
        if (0xD800 <= i) && (i <= 0xDFFF)
        {
            continue;
        }
        buffer.push(0b11100000 + ((i >> 12) as u8));
        buffer.push(0b10000000 + (((i >> 6) & 0b00111111) as u8));
        buffer.push(0b10000000 + ((i & 0b00111111) as u8));       
    } 
    // four byte Unicode codepoints  
    for i in 0x10000..0x110000
    {
        buffer.push(0b11110000 + (i >> 18) as u8);
        buffer.push(0b10000000 + ((i >> 12) & 0b00111111) as u8);
        buffer.push(0b10000000 + ((i >> 6) & 0b00111111) as u8);
        buffer.push(0b10000000 + (i & 0b00111111) as u8);        
    }
    output_file_handle.write_all(&buffer).unwrap();
    
    return;
}