package tools.safepatch.cfgimpl;

import java.util.HashMap;
import java.util.Map;

import cfg.C.CCFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff;

public class ASTCFGMapping extends CFGMapping {

	private FunctionDiff fDiff;
	private ASTDiff astDiff;
	private Map<CFGNode, CFGNode> srcDstMap;
	private Map<CFGNode, CFGNode> dstSrcMap;
	
	public ASTCFGMapping(FunctionDiff diff) {
		this.fDiff = diff;
		this.srcCfg = this.fDiff.getOriginalGraphs().getCfg();
		this.dstCfg = this.fDiff.getNewGraphs().getCfg();
		this.astDiff = this.fDiff.getFineGrainedDiff();
	}

	/**
	 * Performs the mapping
	 */
	public void map(){
		if(this.srcDstMap == null && this.dstSrcMap == null){
			this.srcDstMap = new HashMap<>();
			this.dstSrcMap = new HashMap<>();
	
			//First maps entry, exit and error nodes
			this.addMapping(this.srcCfg.getEntryNode(), this.dstCfg.getEntryNode());
			this.addMapping(this.srcCfg.getErrorNode(), this.dstCfg.getErrorNode());
			this.addMapping(this.srcCfg.getExitNode(), this.dstCfg.getExitNode());
			
			//Map exception node if present
			if(this.srcCfg instanceof CCFG && this.dstCfg instanceof CCFG)
				if(((CCFG) this.srcCfg).hasExceptionNode() && ((CCFG) this.dstCfg).hasExceptionNode())
					this.addMapping(((CCFG) this.srcCfg).getExceptionNode(), ((CCFG) this.dstCfg).getExceptionNode());
				
			//Map all the other nodes according to the given AST mapping
			for(CFGNode srcNode : this.srcCfg.getVertices()){
				for(CFGNode dstNode : this.dstCfg.getVertices()){
					if(srcNode instanceof ASTNodeContainer && dstNode instanceof ASTNodeContainer){
						ASTNodeContainer src = (ASTNodeContainer) srcNode;
						ASTNodeContainer dst = (ASTNodeContainer) dstNode;
						if(this.astDiff.getMappedDstNode(src.getASTNode()) != null
								&& this.astDiff.getMappedDstNode(src.getASTNode()).equals(dst.getASTNode()))
							this.addMapping(srcNode, dstNode);
					}
				}
			}
		}
	}
	
	private void addMapping(CFGNode src, CFGNode dst){
		this.srcDstMap.put(src, dst);
		this.dstSrcMap.put(dst, src);
	}
	
	@Override
	public CFGNode getMappedSrcNode(CFGNode dstNode) {
		if(dstSrcMap == null)
			throw new RuntimeException("Mapping not generated, call map() method before.");
		return this.dstSrcMap.get(dstNode);
	}

	@Override
	public CFGNode getMappedDstNode(CFGNode srcNode) {
		if(srcDstMap == null)
			throw new RuntimeException("Mapping not generated, call map() method before.");
		return this.srcDstMap.get(srcNode);
	}

}
