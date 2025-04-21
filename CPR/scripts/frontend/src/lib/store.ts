import { writable } from 'svelte/store';
import type { NodeType, EdgeType, GraphType } from './graph_api';
import type { Metadata } from './metadata';
export interface AnalysisTableType {
  columns: number[];
  rows: {
    base: number;
    row: boolean[];
  }[];
}
export interface StateType {
  state: number;
  crashId: number;
  patchId: number;
  stateType: string;
  isCrash: boolean;
  actuallyCrashed: boolean;
  exitLoc: string;
  exit: string;
  regressionTrace: string;
  stackTrace: string;
}

export interface ResultType {
  state_map: { key: number; value: StateType }[];
  removed_if_feasible: { key: number; value: Array<[number, number]> }[];
  removed_if_infeasible: { key: number; value: Array<[number, number]> }[];
  removed: { key: number; value: Array<[number, number]> }[];
  crash_id_to_state: [number, number][];
  crash_test_result: Record<number, Array<number>>;
  graph: { nodes: Set<number>; edges: Set<[number, number, string]> };
  patch_analysis: Record<number, Array<number>>;
  table: AnalysisTableType;
}

export interface InputSelectType {
  input: number;
  feasibility: boolean;
  used: boolean;
}

export interface DirDataType {
  dir: string;
  inputs: InputSelectType[];
}

export const metaDataStore = writable<Metadata>({
  id: 0,
  bug_id: '',
  benchmark: '',
  subject: '',
  vars: [],
  buggy: { code: '', id: '' },
  correct: { code: '', id: '' },
  target: '',
});
export const mdTableStore = writable({ table: '' });
export const graphStore = writable<GraphType>({ nodes: [], edges: [] });
export const resultStore = writable<ResultType>({
  state_map: {},
  removed_if_feasible: {},
  removed_if_infeasible: {},
  removed: {},
  crash_id_to_state: {},
  crash_test_result: {},
  graph: { nodes: new Set(), edges: new Set() },
  patch_analysis: {},
  table: { columns: [], rows: [] },
});
export const dirDataStore = writable<DirDataType>({ dir: '', inputs: [] });
