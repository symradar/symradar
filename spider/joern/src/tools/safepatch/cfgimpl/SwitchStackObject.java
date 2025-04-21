package tools.safepatch.cfgimpl;

import java.util.ArrayList;
import java.util.List;

import cfg.CFGEdge;
import cfg.nodes.ASTNodeContainer;

public class SwitchStackObject extends StackObject {

	private List<CFGEdge> caseEdges;
	private List<ASTNodeContainer> caseNodes;
	
	public SwitchStackObject(ASTNodeContainer conditionNode, List<CFGEdge> caseEdges) {
		super(conditionNode);
		this.caseEdges = caseEdges;
		this.caseNodes = new ArrayList<ASTNodeContainer>();
		this.caseEdges.stream().forEach(edge -> this.caseNodes.add((ASTNodeContainer) edge.getDestination()));
	}

	public List<CFGEdge> getCaseEdges() {
		return caseEdges;
	}

	public List<ASTNodeContainer> getCaseNodes() {
		return caseNodes;
	}
	
}
