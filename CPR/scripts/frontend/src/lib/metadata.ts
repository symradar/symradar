//   {
//     "id": 1,
//     "bug_id": "CVE-2016-5321",
//     "benchmark": "extractfix",
//     "subject": "libtiff",
//     "vars": [
//       "x",
//       "y"
//     ],
//     "buggy": {
//       "code": "(1)",
//       "id": "buggy"
//     },
//     "correct": {
//       "code": "(x < y)",
//       "id": "2-0"
//     },
//     "target": "readSeparateTilesIntoBuffer"
//   },
export interface Metadata {
  id: number;
  bug_id: string;
  benchmark: string;
  subject: string;
  vars?: string[];
  buggy?: {
    code: string;
    id: string;
  };
  correct?: {
    code: string;
    id: string;
  };
  target?: string;
}
