package tools.safepatch.cfgimpl;

import cfg.CFG;
import cfg.nodes.CFGNode;

public abstract class CFGMapping {

	public CFG srcCfg;
	public CFG dstCfg;
	
	
	public CFG getSrcCfg() {
		return srcCfg;
	}

	public CFG getDstCfg() {
		return dstCfg;
	}

	/**
	 * Given a node from the dst CFG returns the mapped
	 * node in the src CFG, or null if no mapping is present.
	 */
	public abstract CFGNode getMappedSrcNode(CFGNode dstNode);
	
	/**
	 * Given a node from the dst CFG returns the corresponding
	 * node in the src CFG, or null if no mapping is present.
	 */
	public abstract CFGNode getMappedDstNode(CFGNode srcNode);
	
	/**
	 * Given a node from the src/dst CFG returns the corresponding
	 * node in the dst/src CFG, or null if no mapping is present.
	 */
	public CFGNode getMappedNode(CFGNode n){
		CFGNode src = getMappedSrcNode(n);
		CFGNode dst = getMappedDstNode(n);
		if(src != null && dst != null)
			throw new RuntimeException("Mapping error.");
		else if (src != null) return src;
		else if (dst != null) return dst;
		else return null;
	}
	
	
}
