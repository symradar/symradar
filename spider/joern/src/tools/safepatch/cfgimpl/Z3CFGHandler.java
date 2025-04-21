package tools.safepatch.cfgimpl;

import java.util.Stack;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.Context;

import ast.ASTNode;
import ast.statements.Condition;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;

/**
 * @author Eric Camellini
 *
 */
public class Z3CFGHandler implements DoubleCFGNodeHandler {

	/*
	 * TODO:
	 * - Implement generation of Z3 constraints for every kind of node 
	 * - Implement the CFG diff to Z3 conversion as discussed
	 */
	
	private static final Logger logger = LogManager.getLogger();
	
	private Stack<ASTNodeContainer> originalCFGConditionsStack;
	private Stack<ASTNodeContainer> newCFGConditionsStack;
	
	public Z3CFGHandler() {
		this.originalCFGConditionsStack = new Stack<ASTNodeContainer>();
		this.newCFGConditionsStack = new Stack<ASTNodeContainer>();
	}
	
	@Override
	public void handleOriginal(CFGNode node) {
		if(logger.isDebugEnabled()) logger.debug("Original node: {}", node);
		if(node instanceof ASTNodeContainer){
			ASTNodeContainer n = (ASTNodeContainer) node;
			//We first pop from the stack the conditions that don't hold anymore
			popStack(n, OriginalOrNew.ORIGINAL);
			
			//We handle the node
			
			//We push it on the stack if necessary
			pushStack(n, OriginalOrNew.ORIGINAL);
			
		}
	}

	@Override
	public void handleNew(CFGNode node) {
		if(logger.isDebugEnabled()) logger.debug("New node: {}", node);
		if(node instanceof ASTNodeContainer){
			ASTNodeContainer n = (ASTNodeContainer) node;
			//We first pop from the stack the conditions that don't hold anymore
			popStack(n, OriginalOrNew.NEW);
			
			//We handle the node
			
			//We push it on the stack if necessary
			pushStack(n, OriginalOrNew.NEW);
		}
	}
	
	@Override
	public void endOriginal() {
		if(logger.isDebugEnabled()) logger.debug("Original CFG end.");
		this.originalCFGConditionsStack.clear();
	}

	@Override
	public void endNew() {
		if(logger.isDebugEnabled()) logger.debug("New CFG end.");
		this.newCFGConditionsStack.clear();
	}
	
	private static enum OriginalOrNew { 
		ORIGINAL,
		NEW
	}
	
	private void popStack(ASTNodeContainer node, OriginalOrNew originalOrNew) {		
		/*
		 * POPPING PHASE:
		 * 
		 * We have also to check if we exited from the sub-CFG that is all
		control-dependent from the condition on the peak of the stack.
		We do it until we find a condition under which the node is still contained
		or until the stack is empty
		
		TODO: 
			- POP also in Z3 ?
			
		 */	
		do{
			ASTNodeContainer peek = null;
			if(originalOrNew == OriginalOrNew.ORIGINAL){
				if(!this.originalCFGConditionsStack.isEmpty())
					peek = this.originalCFGConditionsStack.peek();
			} else {
				if(!this.newCFGConditionsStack.isEmpty())
					peek = this.newCFGConditionsStack.peek();
			}

			if(peek != null &&
					!peek.getASTNode().getParent().getDescendants().contains(node.getASTNode())){
				if(originalOrNew == OriginalOrNew.ORIGINAL){
					if(logger.isDebugEnabled()) logger.debug("Original stack: popping {}", peek);
					this.originalCFGConditionsStack.pop();
				} else {
					if(logger.isDebugEnabled()) logger.debug("New stack: popping {}", peek);
					this.newCFGConditionsStack.pop();
				}
			} else {
				break;
			}
		} while(true);
	}
	
	private void pushStack(ASTNodeContainer node, OriginalOrNew originalOrNew){
		/*
		 * PUSHING PHASE
		 * If the node is a condition we push it on the stack
		 * 
		 *  TODO:
		 *  	- Push also in Z3?
		 */
		
		ASTNode n = node.getASTNode();
		
		if(n instanceof Condition){
			//Whenever we find a Condition in the CFG we push it on the stack
			if(originalOrNew == OriginalOrNew.ORIGINAL){ 
				this.originalCFGConditionsStack.push(node);
				if(logger.isDebugEnabled()) logger.debug("Original stack: pushing {}", node);
			} else { 
				this.newCFGConditionsStack.push(node);
				if(logger.isDebugEnabled()) logger.debug("New stack: pushing {}", node);
			}
		}
	}
}
