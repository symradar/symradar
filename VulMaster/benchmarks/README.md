# Evaluation

## Evaluate ExtractFix

To evaluate ExtractFix, follow the steps below:

1. **Generate a dataset**:

Run following command to generate a dataset for evaluation:
```bash
cd benchmarks/extractfix
./setup.sh
```
This will generate `extractfix.json` in `VulMaster/setup`.

2. **Run VulMaster**:

Run the following command to run VulMaster:
```bash
./run-vulmaster.sh
```
This will generate `result` directory in `benchmarks/extractfix` with the evaluation results.

* `run.log`: Log file of VulMaster.
* `test_results/result.json`: Result JSON file of VulMaster.