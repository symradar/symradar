project_name=coreutils
bug_id=gnubug-25003
dir_name=$1/extractfix/$project_name/$bug_id

project_url=https://github.com/coreutils/coreutils.git
commit_id=68c5eec


current_dir=$PWD
mkdir -p $dir_name
cd $dir_name
git clone $project_url src
cd src
git checkout $commit_id

sed -i '987i klee_assert(initial_read > start);' src/split.c
sed -i '987i CPR_OUTPUT("obs", "i32", initial_read - start);\n' src/split.c
sed -i '985d' src/split.c
sed -i '985i if(__cpr_choice("L290", "bool", (int[]){start, initial_read, bufsize}, (char*[]){"start","initial_read", "bufsize"}, 3, (int*[]){}, (char*[]){}, 0))' src/split.c
sed -i '97i #ifndef CPR_OUTPUT\n#define CPR_OUTPUT(id, typestr, value) value\n#endif' src/split.c
sed -i '97i #include <klee/klee.h>' src/split.c
git add src/split.c
git commit -m "instrument cpr"

./bootstrap
FORCE_UNSAFE_CONFIGURE=1 CC=$CPR_CC CXX=$CPR_CXX ./configure CFLAGS='-g -O0 -static -fPIE' CXXFLAGS="$CFLAGS"
make CFLAGS="-fPIC -fPIE -L/root/projects/uni-klee/build/lib  -lkleeRuntest -I/root/projects/uni-klee/include" CXXFLAGS=$CFLAGS -j32
make CFLAGS="-fPIC -fPIE -L/root/projects/uni-klee/build/lib  -lkleeRuntest -I/root/projects/uni-klee/include" CXXFLAGS=$CFLAGS src/split -j32


cd $current_dir
cp repair.conf $dir_name
cp spec.smt2 $dir_name
cp t1.smt2 $dir_name
cp -rf components $dir_name

