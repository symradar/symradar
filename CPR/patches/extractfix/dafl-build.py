import subprocess
import multiprocessing as mp
import sys

CORRECT_PATCHES=(
    # 'binutils/CVE-2017-15025',
    # 'binutils/CVE-2018-10372',
    # 'coreutils/bugzilla-19784',
    # 'coreutils/bugzilla-26545',
    'coreutils/gnubug-25003',
    'coreutils/gnubug-25023',
    'jasper/CVE-2016-8691',
    # 'jasper/CVE-2016-9387',
    'libjpeg/CVE-2012-2806',
    'libjpeg/CVE-2017-15232',
    # 'libjpeg/CVE-2018-14498',
    # 'libjpeg/CVE-2018-19664',
    # 'libtiff/bugzilla-2611',
    'libtiff/bugzilla-2633',
    # 'libtiff/CVE-2014-8128',
    # 'libtiff/CVE-2016-3186',
    # 'libtiff/CVE-2016-3623',
    # 'libtiff/CVE-2016-5314',
    'libtiff/CVE-2016-5321',
    # 'libtiff/CVE-2016-9273',
    'libtiff/CVE-2016-10094',
    # 'libtiff/CVE-2017-7595',
    'libtiff/CVE-2017-7601',
    'libxml2/CVE-2012-5134',
    # 'libxml2/CVE-2016-1834',
    'libxml2/CVE-2016-1838',
    # 'libxml2/CVE-2016-1839',
    # 'libxml2/CVE-2017-5969',
)

def run(sub):
    print(f'Applying {sub}...')
    res=subprocess.run('./afl-init.sh', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=sub,shell=True)
    with open(f'{sub}/afl-init.log', 'wb') as f:
        f.write(res.stdout)
    if res.returncode != 0:
        print(f'Error applying {sub}!')
        return
    print(f'Finished applying {sub}')

pool=mp.Pool(int(sys.argv[1]))
for sub in CORRECT_PATCHES:
    pool.apply_async(run, (sub,))
pool.close()
pool.join()