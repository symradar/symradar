# VulMaster

This repository is the replication package of **"Out of Sight, Out of Mind: Better Automatic Vulnerability Repair by Broadening Input Ranges and Sources"**.


## Resources

* In the replication, we provide:
  * the scripts we used to:
    * `0a_train.sh`: fine-tune the models with validation.
    * `0b_test.sh`:  perform inference using the fine-tuned models.
  * the source code we used to:
    * `train_model.py`: the main code for training/validating.
    * `test_model.py`: the main code for testing.
    * `src/`: the utils, model, data preprocessing, etc. for FiD.
  * the CWE knowledge we collected from CWE website:
    * `CWE_examples_GPT35_generate_fixes_full.csv`: the raw vulnerable code examples and their analysis and CWE names directly collected from CWE homepages.
    * `chatgpt_api_generate_fix.py`: the main code for generating fixes for vulnerable code examples with expert analysis as guidance.
    * `ChatGPT_generated_fixes_labels.xlsx`: the manually labeled correctness for generated fixes for vulnerable code examples.
   
      
 We stored the datasets you need in order to replicate our experiments at: https://zenodo.org/records/10150013 and [Here](https://drive.google.com/drive/folders/1L5fkJ_J-NvuWlcr-GbfomorxoS6HwuTs?usp=sharing) is CodeT5 model after adaptation. 
 
* `requirements.txt` contains the dependencies needed.

* The experiments were conducted on a server equipped with NVIDIA L40 GPU and Intel(R) Xeon(R) CPU E5-2420 v2@ 2.20GHz, running the Ubuntu OS.
  
* If you meet OutOfMemoryError: please note that you typically need around 30 GB GPU memory to run VulMaster.


## Install dependencies

To install dependencies in once, run:
```bash
./setup.sh
```

Then, install conda env and dependencies:
```bash
conda create -n vulmaster python=3.9 
conda activate vulmaster
pip install -r requirements.txt
```
## Train and Test 

To replicate VulMaster, ensure that `c_dataset/` is in the root path of this project. 

Training:
```bash
./0a_train.sh 
```

Testing:
```bash
./0b_test.sh
```

## Evaluation

To evaluate ExtractFix, look [readme in benchmarks](benchmarks/README.md) for details.

## Example of input json file
```json
[
  {
    "questions": "CWE-(ID) Code Input Vulnerable Code Is: CWE-(ID) (vulnerable function with <vul-start> and <vul-end>)",
    "answers": [
      "CWE-(ID) Fixed Code Lines are: <vul-start> (fixed code by developer) [<vul-end>]"  // If <vul-end> not exist, it will insert fixed code before original <vul-start>
    ],
    "ctxs": [
      {
        "id": "0",
        "title": "CWE-(ID) Code Input Vulnerable Code Is: CWE-(ID) (vulnerable function with <vul-start> and <vul-end>)",
        "text": "CWE-(ID) Fixed Code Lines are:"
      },
      {
        "id": "X02/X03/...", // X is the number of the subject e.g. 2/3/,..., 102/103/...
        "title": "CWE-(ID) Code Input AST Vulnerable Code Is: (AST segments of patch location in vulnerable function)",
        "text": "CWE-(ID) Fixed Code Lines are:"
      },
      {
        "id": "10000/10001/10002/...",
        "title": "CWE-(ID) Vulnerable Code Is: CWE-(ID) (CWE example: vulnerable code with <vul-start> and <vul-end>)",
        "text": "CWE-(ID) Fixed Code Lines are: <vul-start> (CWE example: fixed code by developer) [<vul-end>]"
      },
      {
        "id": "40000",
        "title": "CWE-(ID) Name: (CWE name)\tDescription: (CWE description)\tRelated Weakness: (CWE related weakness)\tObserved Examples: (CWE examples with link)",
        "text": "CWE-(ID) Fixed Code Lines are:"
      }
    ]
  }
]
```

## Generate fixes for CWE vulnerable code examples via ChatGPT
```
python chatgpt_api_generate_fix.py
```

