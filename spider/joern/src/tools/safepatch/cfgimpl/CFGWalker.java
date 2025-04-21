package tools.safepatch.cfgimpl;

import java.util.ArrayList;

import cfg.CFG;
import cfg.nodes.CFGNode;

public class CFGWalker {

	/*
	 * This class walks the Joern CFG
	 * and passes every node to the handler.
	 * 
	 * Every node is traversed only once.
	 * (i.e. all loops are traversed once)
	 */
	
	private CFG cfg;
	private CFGIterator walker;
	private CFGNodeHandler handler;

	public CFGWalker(CFG cfg, CFGNodeHandler handler) {
		this.cfg = cfg;
		this.walker = new CFGIterator(this.cfg);
		this.handler = handler;
	}
	
	/**
	 * Starts traversing the CFG passing every node to the handler.
	 */
	public void traverse(){
		
		CFGNode current = this.walker.getCurrentNode();
		while(current != null){
			
			this.handler.handleNode(current);
			
			if(this.walker.hasNext()){
				walker.moveToNext();
				current = walker.getCurrentNode();
			} else current = null;
		}
	}
	
}
