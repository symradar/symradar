# Spider

This repo is originated from Aravind Machiry's paper published in SP 2020: [SPIDER: Enabling Fast Patch Propagation In Related Software Repositories](https://doi.org/10.1109/SP40000.2020.00038). Use the following BibTex to cite the paper:
```
@INPROCEEDINGS{9152613,
  author={Machiry, Aravind and Redini, Nilo and Camellini, Eric and Kruegel, Christopher and Vigna, Giovanni},
  booktitle={2020 IEEE Symposium on Security and Privacy (SP)}, 
  title={SPIDER: Enabling Fast Patch Propagation In Related Software Repositories}, 
  year={2020},
  volume={},
  number={},
  pages={1562-1579},
  keywords={Security;Kernel;Testing;Databases;Androids;Humanoid robots},
  doi={10.1109/SP40000.2020.00038}}
```

## Pre-requisites
- JDK 8
- ant
- unifdef
- libz3

To install every pre-requisites, run the following command:
```bash
sudo apt-get install openjdk-8-jdk ant unifdef libz3-dev libz3-java
```

## Installation
To build the Spider, run the following command:
```bash
ant
```
After the build is successful, you can find the `executable.jar` in the root directory.

## Usage
To run the Spider in default options, run the following command:
```bash
java -jar executable.jar <old source file> <new source file>
```

For example, to run the Spider with the `q6asm.c` provided in examples, run the following command:
```bash
java -jar executable.jar examples/other/no_cve_1/old_latest/q6asm.c examples/other/no_cve_1/new/q6asm.c
```

After running the command, you will see the following output:
```
ERROR StatusLogger No log4j2 configuration file found. Using default configuration: logging only errors to the console.
[{"function":"q6asm_audio_client_buf_alloc_contiguous","originalLine":1019,"newLine":1019,"heuristics":{"C_SENS_FUN":false,"INSERTED_IFS":true,"INIT_OR_ERROR":false,"REMOVED_STUFF":false,"KERNEL_SYNCH":false,"INSERTED_STMT":false,"OFF_BY_ONE":false,"KNOWN_PATCHES":false,"CONDITIONS":false,"MODIFIED_CONDITIONS":false,"FLOW_RESTRICTIVE":true},"features":{"INSERTED_IFS_COUNT":1,"WIDENED_CONDITIONS_COUNT":0,"CHANGED_CONDITIONS_COUNT":0,"RESTRICTED_CONDITIONS_COUNT":0,"BI_IMPLICATIONS_COUNT":0,"INSERTED_STMTS_COUNT":0},"security":true}]
```

To check the patch is safe, check the `security` field in the output. If the `security` field is `true`, the patch is safe.
In this example, the patch is safe.

:warning: **Note**: Spider preprocessor will remove if the patched location is wapped by `#ifdef` and `#endif`. Remove them before running the Spider.

## Evaluating Extractfix

We prepared Python scripts to evaluate Extractfix benchmarks.

### Setup
First, setup the benchmark by running the following command:
```bash
cd extractfix
python3 gen-patches.py
```
This will read `meta-program.json` and generate patched sources in `<project>/<subject>/patches` directory for every subject.

### Run
Then, run Spider to evaluate the patches by running the following command:
```bash
python3 run_experiment.py <# of processes>
```
This will run Spider in parallel for extractfix subjects.
The `<# of processes>` is the number of processes to run Spider in parallel.

It will run Spider for every patched source in `<project>/<subject>/patches` directory and generate the evaluation results in `<project>/<subject>/output/results.json`.
In this file, `true` represents the patch is safe and `false` represents the patch is unsafe.

### Parse Result
To parse the results into a single JSON and CSV file, run the following command:
```bash
python3 analyze-result.py
```
This will generate `final_result.json` and `final_result.csv` in `extractfix/` directory.

In `final_result.json`, you can see the evaluation results and statistics for each subject and overall.
```
{
  "result_each_subject": {
      "<subject>": {
        "true_correct": <int>,
        "true_incorrect": <int>,
        "false_correct": <int>,
        "false_incorrect": <int>,
        "total_patches": <int>
      },
      ...
  },
  "statistic_each_subject": {
    "<subject>": {
      "correct_rate": <float>,
      "incorrect_rate": <float>
    },
    ...
  },
  "result_overall": {
    "true_correct": <int>,
    "true_incorrect": <int>,
    "false_correct": <int>,
    "false_incorrect": <int>
  },
  "statistic_overall": {
    "correct_rate": <float>,
    "incorrect_rate": <float>
  },
  "total_patches": <int>
}
```

* `result_each_subject`: The evaluation results for each subject.
  * `true_correct`: The number of *safe* patches that are *correctly* evaluated.
  * `true_incorrect`: The number of *safe* patches that are *incorrectly* evaluated.
  * `false_correct`: The number of *unsafe* patches that are *correctly* evaluated.
  * `false_incorrect`: The number of *unsafe* patches that are *incorrectly* evaluated.
  * `total_patches`: The total number of patches for the subject.
* `statistic_each_subject`: The statistics for each subject.
  * `correct_rate`: The rate of *correct* patches that Spider identified *successfully*.
  * `incorrect_rate`: The rate of *incorrect* patches that Spider identified *successfully*.
* `result_overall`: The evaluation results for all subjects.
* `statistic_overall`: The statistics for all subjects.
* `total_patches`: The total number of patches for all subjects.

In `final_result.csv`, you can see the evaluation results and statistics in CSV format.

Below is the description of the columns in the CSV file:
* `Subject`: The subject name.
* `True Correct`: The number of *safe* patches that are *correctly* evaluated.
* `False Correct`: The number of *unsafe* patches that are *correctly* evaluated.
* `True Incorrect`: The number of *safe* patches that are *incorrectly* evaluated.
* `False Incorrect`: The number of *unsafe* patches that are *incorrectly* evaluated.
* `Total Patches`: The total number of patches for the subject.
* `Correct Success Rate`: The rate of *correct* patches that Spider identified *successfully*.
* `Incorrect Success Rate`: The rate of *incorrect* patches that Spider identified *successfully*.

The last row of the CSV file shows the statistics for all subjects.

### Heuristics used in Spider

Spider uses six heuristics to identify the safe patches in default: Known_patch, Modified_conditions, Inserted_ifs, Init_or_error, Conditions, and Flow_restrictive.
Spider identifies the patch is *safe* when *any* of the heuristics is satisfied.
Below is the description of the heuristics.

In the description, *Error handling block* represents the block to handle the error.
In Spider, it only contains return statement and returns literal or goto statement to the label that the name is related to error (e.g. error, err, panic, fatal).
Please note that the other common statements such as `exit` or `break`, or returning variable are not considered as *Error handling block*.

:warning: **Note**: Description below is written in my hand based on the source code and paper. It may incorrect. Any suggestion or correction is welcome.

1. **Known_patch**: The patch affects function calls that are known to be safe ONLY. The functions are:
  * `scanf`, `printf`, `lock` and `unlock` functions
  * `memcpy`, `strcpy`, `strncpy`, and `strlcpy` functions
2. **Modified_conditions**: If the patch modifies the conditions, it satisfies the following criteria:
  * If the condition is restricted, it may execute then branch lesser. Then block should *NOT* be Error handling block.
    If the condition is not in the if statement, it is always safe.
  * If the condition is widened, it may execute then branch more. Then block should be Error handling block.
    If the condition is not in the if statement, it is always unsafe.
  * If the condition doesn't imply between them, it is always unsafe (e.g. `x > 1` and `x < -1`).
  * If the condition bi-implied, it is always safe (e.g. `x > 1` and `x < 5`).
3. **Inserted_ifs**: If the patch inserts the if statement, it satisfies the following criteria:
  * If the if statement does NOT have else branch, it should satisfy any of the following criteria:
    * Then block is Error handling block;
    * Then block is simply moved from another location;
    * Then block is another if statement that satisfies *Inserted_ifs*.
  * If the if statement has else branch, it should satisfy any of the following criteria:
    * One of then or else block is Error handling block and the other is simply moved from another location;
    * Both then and else branch are if statements that satisfy *Inserted_ifs*.
4. **Init_or_error**: The patch modifies only variable initialization or error handling block.
5. **Conditions**: If newly inserted if statement and the condition in another location is modified, it satisfies the following criteria:
  * New if statement should satisfy *Inserted_ifs*.
  * Modified condition should not affect the condition of the new if statement.
  * Modified condition should satisfy *Modified_conditions*.
6. **Flow_restrictive**: The patch satisfies the heuristics mentioned in the paper and the following criteria:
  * If the condition is modified, it should satisfy *Modified_conditions*.
  * If an if statement is inserted, it should satisfy *Inserted_ifs*.

For **Flow_restrictive** heuristic, details are in the paper.
<!-- **Command to check the diff:**
```bash
git diff --no-index old_latest/ndisc.c new/ndisc.c
```

README to be written (should include Z3 installation instructions, python venv requirements, etc.) -->
