package tools.safepatch.cfgimpl;

import java.util.ArrayList;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import cfg.CFG;
import cfg.nodes.CFGNode;
import tools.safepatch.diff.FunctionDiff;

public class DoubleCFGWalker {

	private static final Logger logger = LogManager.getLogger();
	
	FunctionDiff fDiff;
	
	CFG originalCfg;
	CFG newCfg;
	CFGIterator originalWalker;
	CFGIterator newWalker;
	CFGDiffProvider cfgDiff;
	DoubleCFGNodeHandler handler;
	List<CFGNode> traversedNodes;
	
	public DoubleCFGWalker(CFG originalCfg, CFG newCfg, CFGDiffProvider cfgDiff, DoubleCFGNodeHandler handler) {
		this.originalCfg = originalCfg;
		this.newCfg = newCfg;
		this.originalWalker = new CFGIterator(this.originalCfg);
		this.newWalker = new CFGIterator(this.newCfg);
		this.cfgDiff = cfgDiff;
		this.handler = handler;
		this.traversedNodes = new ArrayList<>();
	}

	public void traverse(){
		
		if(!traversedNodes.contains(originalWalker.getCurrentNode()) &&
				isDeletedOrInserted(originalWalker.getCurrentNode())){
				traverseDeletedBlock();
				traverse();
		} else if (!traversedNodes.contains(newWalker.getCurrentNode()) &&
				isDeletedOrInserted(newWalker.getCurrentNode())){
				traverseInsertedBlock();
				traverse();
		} else {
			if(!traversedNodes.contains(originalWalker.getCurrentNode())){
				handler.handleOriginal(originalWalker.getCurrentNode());
				this.traversedNodes.add(originalWalker.getCurrentNode());
				if(!originalWalker.hasNext()) handler.endOriginal();
			}
			if(!traversedNodes.contains(newWalker.getCurrentNode())){
				handler.handleNew(newWalker.getCurrentNode());
				this.traversedNodes.add(newWalker.getCurrentNode());
				if(!newWalker.hasNext()) handler.endNew();
			}
			
			if(originalWalker.hasNext() || newWalker.hasNext()){
				if(originalWalker.hasNext()) originalWalker.moveToNext();
				if(newWalker.hasNext()) newWalker.moveToNext();
				traverse();
			}
			
		}
		
	}


	private void traverseInsertedBlock() {
		while(isDeletedOrInserted(newWalker.getCurrentNode())){
			if(logger.isDebugEnabled()) logger.debug("INSERTED:");
			handler.handleNew(newWalker.getCurrentNode());
			traversedNodes.add(newWalker.getCurrentNode());
			if(newWalker.hasNext())
				newWalker.moveToNext();
			else{
				handler.endNew();
				break;
			}
		}
	}

	private void traverseDeletedBlock() {
		while(isDeletedOrInserted(originalWalker.getCurrentNode())){
			if(logger.isDebugEnabled()) logger.debug("DELETED:");
			handler.handleOriginal(originalWalker.getCurrentNode());
			traversedNodes.add(originalWalker.getCurrentNode());
			if(originalWalker.hasNext())
				originalWalker.moveToNext();
			else {
				handler.endOriginal();
				break;
			}
		}
	}
	
	private boolean isDeletedOrInserted(CFGNode node){
		return cfgDiff.isDeleted(node) || cfgDiff.isInserted(node);
	}
}

