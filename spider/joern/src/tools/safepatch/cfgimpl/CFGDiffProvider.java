package tools.safepatch.cfgimpl;

import cfg.nodes.CFGNode;

public interface CFGDiffProvider {
	
	/**
	 * Returns true if the node is marked as removed from that CFG.
	 */
	public boolean isDeleted(CFGNode n);
	
	/**
	 * Returns true if the node is marked as inserted into that CFG.
	 */
	public boolean isInserted(CFGNode n);
	
}
