bug_id=bubblesort
dir_name=$1/svcomp/array/$bug_id
project_url=https://github.com/sosy-lab/sv-benchmarks.git
program_dir=/root/projects/CPR/data/svcomp/sv-benchmarks/c/array-examples
bench_dir=/root/projects/CPR/data/svcomp/sv-benchmarks
[ ! -d $bench_dir ] && git clone $project_url $bench_dir
mkdir -p $1/svcomp/config
current_dir=$PWD

cp sorting_bubblesort_ground-2.c $program_dir
cd $program_dir
make CXX=$CPR_CXX CC=$CPR_CC  LDFLAGS="-lcpr_runtime -L/root/projects/CPR/lib -L/root/projects/uni-klee/build/lib  -lkleeRuntest -I/root/projects/uni-klee/include" -j32 sorting_bubblesort_ground-2

cd $current_dir
mkdir -p $dir_name
cp repair.conf $dir_name
cp spec.smt2 $dir_name
cp t1.smt2 $dir_name
cp -rf components $dir_name

