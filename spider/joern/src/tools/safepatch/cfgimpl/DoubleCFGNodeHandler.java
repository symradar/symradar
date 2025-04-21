package tools.safepatch.cfgimpl;

import cfg.nodes.CFGNode;

/**
 * @author Eric Camellini
 *
 */
public interface DoubleCFGNodeHandler {
	
	/**
	 * Handles the original node.
	 */
	public void handleOriginal(CFGNode node);
	
	
	/**
	 * Handles the new node.
	 */
	public void handleNew(CFGNode node);
	
	/**
	 * Method to call at the end of a the original CFG traversal
	 */
	public void endOriginal();
	
	/**
	 * Method to call at the end of a the new CFG traversal
	 */
	public void endNew();
}
