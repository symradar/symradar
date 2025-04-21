package tools.safepatch.cfgimpl;

import ast.ASTNode;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tools.safepatch.fgdiff.ASTDiff;

public class ASTCFGDiffProvider implements CFGDiffProvider {

	ASTDiff diff;
	public ASTCFGDiffProvider(ASTDiff diff) {
		this.diff = diff;
	}
	
	@Override
	public boolean isDeleted(CFGNode n) {
		//A node is considered deleted if it is marked as deleted or moved 
		//(the node itself, without considering information from the children).
		if(n instanceof ASTNodeContainer){
			ASTNode astNode = ((ASTNodeContainer) n).getASTNode();
			return diff.isDeleted(astNode) || diff.isMoved(astNode);
		} else return false;
	}

	@Override
	public boolean isInserted(CFGNode n) {
		//A node is considered deleted if it is marked as deleted or moved 
		//(the node itself, without considering information from the children).
		if(n instanceof ASTNodeContainer){
			ASTNode astNode = ((ASTNodeContainer) n).getASTNode();
			return diff.isInserted(astNode) || diff.isMoved(astNode);
		} else return false;
	}

}
