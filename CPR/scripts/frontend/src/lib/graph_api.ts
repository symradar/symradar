export interface NodeType {
  data: {
    id: string;
    extra: any;
  };
  classes: string[];
  style: any;
}

export interface EdgeType {
  data: {
    id: string;
    source: string;
    target: string;
    extra: any;
  };
  classes: string[];
  style: any;
}

export interface GraphType {
  nodes: NodeType[];
  edges: EdgeType[];
}
