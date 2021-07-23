import os
import sys
import time
import signal

import spotipy
import sqlalchemy
from sqlalchemy.orm import declarative_base

currently_playing_refresh_rate = 60
app_internals = { "client_id": os.getenv("SPOTIPY_CLIENT_ID"), "client_secret": os.getenv("SPOTIPY_CLIENT_SECRET"), "redirect_uri": os.getenv("SPOTIPY_REDIRECT_URI"), "user_scope": ["user-read-playback-state", "user-read-currently-playing", "user-read-private", "playlist-read-private", "playlist-read-collaborative"], "current_playing_refresh_rate": currently_playing_refresh_rate, "playlist_refresh_rate_multiple": (24 * 60 * 60) / currently_playing_refresh_rate, "database_url": os.getenv("DATABASE_URL") }
if None in app_internals.values():
    print("Set environment variables, exiting.", file=sys.stderr)
    exit(1)
start_time = time.monotonic()    
def sigint_handler(signal_id, frame): 
    print("Playback monitored for " + str(int(time.monotonic() - start_time)) + " seconds.", file = sys.stderr)
    print("New playback records: " + str(new_record_count) + " API errors: " + str(api_error_count), file = sys.stderr)
    print("Complete!", file = sys.stderr)
    sys.stderr.flush()
    exit(0)
signal.signal(signal.SIGINT, sigint_handler)    
    
 
database_class_base = declarative_base()    

class SongListenData(database_class_base):
    __tablename__ = "song_listen_data_store"
    song_uid = sqlalchemy.Column(sqlalchemy.String, primary_key = True, nullable = False)
    user_uid = sqlalchemy.Column(sqlalchemy.String, primary_key = True, nullable = False)
    begin_timestamp = sqlalchemy.Column(sqlalchemy.Integer, primary_key = True, nullable = False)
    device_uid = sqlalchemy.Column(sqlalchemy.String, nullable = False) 
    listening_context = sqlalchemy.Column(sqlalchemy.String)
    listening_volume = sqlalchemy.Column(sqlalchemy.Integer)
    in_private_session = sqlalchemy.Column(sqlalchemy.Boolean) 
    repeat_state = sqlalchemy.Column(sqlalchemy.String)
    shuffle_state = sqlalchemy.Column(sqlalchemy.Boolean)

class PlaylistSnapshotData(database_class_base):
    __tablename__ = "playlist_snapshot_data_store"
    playlist_uid = sqlalchemy.Column(sqlalchemy.String, primary_key = True, nullable = False)  
    user_uid = sqlalchemy.Column(sqlalchemy.String, sqlalchemy.ForeignKey("song_listen_data_store.user_uid", ondelete = 'cascade'), primary_key = True, nullable = False)   
    snapshot_id = sqlalchemy.Column(sqlalchemy.String, primary_key = True)
    snapshot_time = sqlalchemy.Column(sqlalchemy.Integer, primary_key = True) 
    follower_count = sqlalchemy.Column(sqlalchemy.Integer)
    track_count = sqlalchemy.Column(sqlalchemy.Integer)
    playlist_name = sqlalchemy.Column(sqlalchemy.String)
    playlist_description = sqlalchemy.Column(sqlalchemy.String)
    is_collaborative = sqlalchemy.Column(sqlalchemy.Boolean)
    is_public = sqlalchemy.Column(sqlalchemy.Boolean)

class PlaylistSnapshotSongsData(database_class_base):
    __tablename__ = "playlist_snapshot_songs_data_store"    
    playlist_uid = sqlalchemy.Column(sqlalchemy.String, sqlalchemy.ForeignKey("playlist_snapshot_data_store.playlist_uid", ondelete = 'cascade'), primary_key = True, nullable = False)
    snapshot_id = sqlalchemy.Column(sqlalchemy.String, sqlalchemy.ForeignKey("playlist_snapshot_data_store.snapshot_id", ondelete = 'cascade'), primary_key = True, nullable = False)
    song_uid = sqlalchemy.Column(sqlalchemy.String, primary_key = True, nullable = False)  
    added_at = sqlalchemy.Column(sqlalchemy.String)
    added_by = sqlalchemy.Column(sqlalchemy.String)

    
database_engine = sqlalchemy.create_engine(app_internals['database_url'], echo = True)
database_class_base.metadata.create_all(database_engine)
database_session_template = sqlalchemy.orm.session.sessionmaker(bind = database_engine)
database_session = database_session_template()

spotify_api_access = spotipy.Spotify(auth_manager = spotipy.SpotifyOAuth(client_id = app_internals['client_id'], client_secret = app_internals['client_secret'], scope = app_internals['user_scope']))  
current_user = spotify_api_access.current_user()      
song_staging = [None, None, True]
new_record_count = 0
api_error_count = 0
tracked_playlists = [ ]
playlist_snapshot_attempt_counter = 0

if __name__== '__main__':
    while True:
        try:
            current_playback = spotify_api_access.current_playback()
        except:
            api_error_count = api_error_count + 1
            print("API Error #" + str(api_error_count), file = sys.stderr)
            sys.stderr.flush()
            continue

        if current_playback is not None:
            current_playback['timestamp'] = int(current_playback['timestamp'] / 1000)
            if song_staging[0] == current_playback['item']['id'] and song_staging[1] == current_playback['timestamp'] and current_playback['is_playing'] == True and song_staging[2] == True:
                database_session.add(SongListenData(song_uid = current_playback['item']['id'], user_uid = current_user['id'], begin_timestamp = current_playback['timestamp'], device_uid = current_playback['device']['id'], listening_context = None if current_playback['context'] == None else current_playback['context']['uri'], listening_volume = current_playback['device']['volume_percent'], in_private_session = current_playback['device']['is_private_session'], repeat_state = current_playback['repeat_state'], shuffle_state = current_playback['shuffle_state']))
                database_session.commit()
                new_record_count = new_record_count + 1
                print("Saved current playback: " + current_playback['item']['name'] + " by ", end = '', file = sys.stderr)
                for artist in current_playback['item']['artists']:
                    print(artist['name'], end =' ', file = sys.stderr) 
                sys.stderr.flush()      
                song_staging[2] = False
            elif (song_staging[0] != current_playback['item']['id'] or song_staging[1] != current_playback['timestamp']) and current_playback['is_playing'] == True:
                song_staging[0] = current_playback['item']['id']
                song_staging[1] = current_playback['timestamp']
                song_staging[2] = True

        if (playlist_snapshot_attempt_counter % app_internals['playlist_refresh_rate_multiple']) == 0:    
            index = 0
            while index < len(tracked_playlists):
                try: 
                    playlist_data = spotify_api_access.playlist(tracked_playlists[index])
                except:
                    api_error_count = api_error_count + 1
                    print("API Error #" + str(api_error_count), file = sys.stderr)
                    sys.stderr.flush()
                    continue
                if len(database_session.query(PlaylistSnapshotData).filter(PlaylistSnapshotData.playlist_uid == tracked_playlists[index]).filter(PlaylistSnapshotData.snapshot_id == playlist_data['snapshot_id']).all()) == 0:
                    database_session.add(PlaylistSnapshotData(playlist_uid = tracked_playlists[index], user_uid = current_user['id'], snapshot_id = playlist_data['snapshot_id'], snapshot_time = int(time.time()), follower_count = playlist_data['followers']['total'], track_count = playlist_data['tracks']['total'], playlist_name = playlist_data['name'], playlist_description = playlist_data['description'], is_collaborative = playlist_data['collaborative'], is_public = playlist_data['public']))
                    offset = 0
                    playlist_songs = spotify_api_access.playlist_tracks(tracked_playlists[index], limit = 100, offset = offset)
                    while len(playlist_songs['items']) > 0:
                        for song in playlist_songs['items']:
                            database_session.add(PlaylistSnapshotSongsData(playlist_uid = tracked_playlists[index], snapshot_id = playlist_data['snapshot_id'], song_uid = song['track']['id'], added_at = song['added_at'], added_by = song['added_by']['id']))
                        offset = offset + len(playlist_songs['items'])       
                        playlist_songs = spotify_api_access.playlist_tracks(tracked_playlists[index], limit = 100, offset = offset)
                    database_session.commit()                        
                    print("Processed " + str(offset) + " songs in the playlist " + playlist_data['name'] + ".", file = sys.stderr)
                    print("Currently at snapshot: " + playlist_data['snapshot_id'], file = sys.stderr)
                    sys.stderr.flush()
                index = index + 1  
            print("Playlists refreshed.")
            sys.stderr.flush()                    

        playlist_snapshot_attempt_counter = playlist_snapshot_attempt_counter + 1 
        time.sleep(app_internals['current_playing_refresh_rate']) 
