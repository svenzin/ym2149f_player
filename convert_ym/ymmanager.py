import sys
import os
import hashlib
import shutil
import ymparser

def log(message):
    print(message)

def usage():
    print("""
YM files management
Usage:
    ymmanager.py COMMAND ARGS

    Available commands: init, import
""")

def error(message):
    print('Error: {}'.format(message))
    exit(1)

try:
    command = sys.argv[1]
except:
    command = 'unknown'

if command == 'init':
    if os.path.exists('ymdb.txt'):
        error("Directory is already initiated")
    if len(sys.argv) != 2:
        error("Too many arguments")
    
    try:
        with open('ymdb.txt', 'w') as f:
            pass
        os.makedirs('.db')
        os.makedirs('ym')
        os.makedirs('ymr')
        os.makedirs('rejected')
        os.makedirs('rejected/hash_exists')
        os.makedirs('rejected/file_exists')
        os.makedirs('rejected/invalid_archive')
        os.makedirs('rejected/invalid_header_type')
        os.makedirs('rejected/conversion_error')
    except:
        os.remove('ymdb.txt')
        os.removedirs('.db')
        os.removedirs('ym')
        os.removedirs('ymr')
        os.removedirs('rejected/hash_exists')
        os.removedirs('rejected/file_exists')
        os.removedirs('rejected/invalid_archive')
        os.removedirs('rejected/invalid_header_type')
        os.removedirs('rejected/conversion_error')
        os.removedirs('rejected')
        error('Error during "init"')

elif command == 'import':
    if not os.path.exists('ymdb.txt'):
        error("Directory is not initiated")
    if len(sys.argv) != 3:
        error("Too many arguments")
    
    ym_filepath = sys.argv[2]
    with open(ym_filepath, 'rb') as ym_file:
        ym_hash = hashlib.sha1(ym_file.read()).hexdigest()
    ym_hash_filepath = os.path.join('.db', ym_hash) 
    (ym_filedir, ym_filename) = os.path.split(ym_filepath)
    if os.path.exists(ym_hash_filepath):
        os.rename(ym_filepath, os.path.join('rejected', 'hash_exists', ym_filename))
        error("YM file {} is already registered (hash = {})".format(ym_filepath, ym_hash))
    
    ym_source = os.path.join('ym', ym_filename)
    ymr_target = os.path.join('ymr', os.path.splitext(ym_filename)[0] + '.ymr')
    if os.path.exists(ym_source) or os.path.exists(ymr_target):
        os.rename(ym_filepath, os.path.join('rejected', 'file_exists', ym_filename))
        error("File already exists")
    
    try:
        log('Import {}'.format(ym_filename))
        with open(ym_hash_filepath, 'w') as id_file:
            id_file.writelines([ym_source, '\n', ymr_target, '\n'])
        shutil.copy(ym_filepath, ym_source)
        ymparser.convert(ym_source, ymr_target)
    except ymparser.ConversionError:
        os.rename(ym_filepath, os.path.join('rejected', 'conversion_error', ym_filename))
        os.remove(ym_hash_filepath)
        os.remove(ym_source)
        os.remove(ymr_target)
    except ymparser.LhaArchiveError:
        os.rename(ym_filepath, os.path.join('rejected', 'invalid_archive', ym_filename))
        os.remove(ym_hash_filepath)
        os.remove(ym_source)
        os.remove(ymr_target)
    except ymparser.YmHeaderTypeError:
        os.rename(ym_filepath, os.path.join('rejected', 'invalid_header_type', ym_filename))
        os.remove(ym_hash_filepath)
        os.remove(ym_source)
        os.remove(ymr_target)
    except:
        os.remove(ym_hash_filepath)
        os.remove(ym_source)
        os.remove(ymr_target)
        error('Error during import')
    
    # log('Remove original file')
    os.remove(ym_filepath)

else:
    error("Unknown command")