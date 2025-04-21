package tools.safepatch.cfgimpl;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.expressions.AssignmentExpr;
import ast.expressions.Expression;
import ast.statements.Condition;
import cdg.CDG;
import cdg.DominatorTree;
import cfg.CFGEdge;
import cfg.C.CCFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tools.safepatch.util.CFGUtil;

/**
 * This class stacks the conditions in the CFG to save the context.
 * A constraint table is saved built for every condition, so that
 * it can be retrieved and used to enhance the condition implication.
 * 
 * In the current implementation the only conditions that are
 * stacked are if and switch conditions, since all 
 * loops are traversed only once.
 */

public class ConditionsConstraintsExtractor implements CFGNodeHandler {

	private static final Logger logger = LogManager.getLogger();
	
	private CCFG cfg;
	
	private Stack<StackObject> conditionsStack;
	
	// This is to save the state of the current branch
	private String currentBranch = null;
	private Stack<String> currentBranchStack;
	
	private CFGWalker walker;
	
	private Map<String, Constraint> currentConstraints;
	
	private static final String TRUE = "True";
	private static final String FALSE = "False";
	
	public ConditionsConstraintsExtractor(CCFG ccfg) {
		this.cfg = ccfg;
		
		this.conditionsStack = new Stack<StackObject>();
		this.currentBranchStack = new Stack<String>();
		
		this.walker = new CFGWalker(this.cfg, this);
		this.currentConstraints = new HashMap<String, Constraint>();
	}
	
	public void extract(){
		this.walker.traverse();
	}
	
	@Override
	public void handleNode(CFGNode node) {
		
		if(!(node instanceof ASTNodeContainer)){
			if(logger.isErrorEnabled()) logger.error("Handling a non-ASTNodeContainer node");
			return;
		}
		
		ASTNodeContainer n = (ASTNodeContainer) node;
		
		popStacks(n);
		
		updateCurrentBranch(n);
		//TODO add current constraints to node if it is a condition
		
		
		// Handling assignments to add the related constraint.
		if(logger.isDebugEnabled()) logger.debug("Handling node {} (current branch: {})", node, this.currentBranch);
		n.getASTNode().getNodes().forEach(child -> {
			// At the moment we consider only assignments as constraining nodes
			if(child instanceof AssignmentExpr) handleAssignment((AssignmentExpr) child);
		});
		
		pushStacks(n);
		
	}

	private void updateCurrentBranch(ASTNodeContainer n) {
		if(!this.conditionsStack.isEmpty()){
			StackObject peek = this.conditionsStack.peek();
			
			if(peek instanceof IfStackObject){
				if(n.equals(((IfStackObject) peek).getThenNode())){
					if(logger.isDebugEnabled()) logger.debug("First node of true branch for condition {}", peek.getCondition());
					this.currentBranch = TRUE;
				}
				if(n.equals(((IfStackObject) peek).getElseNode())){
					if(logger.isDebugEnabled()) logger.debug("First node of false branch for condition {}", peek.getCondition());
					this.currentBranch = FALSE;
				}
			}
			
			if(peek instanceof SwitchStackObject){
				if(((SwitchStackObject) peek).getCaseNodes().contains(n)){
					if(logger.isDebugEnabled()) logger.debug("First node of {} branch for switch {}", n.getEscapedCodeStr(), peek.getCondition());
					this.currentBranch = n.getEscapedCodeStr();
				}
			}
		} else this.currentBranch = null;
	}

	private void handleAssignment(AssignmentExpr a) {
		if(this.conditionsStack.isEmpty()){
			// In this case it means that this assignment
			// is in the main flow, there cannot be an alternative assignment
			// on another branch (e.g. the else of an if or a switch case)
			addConstraint(a.getLeft().getEscapedCodeStr(), new SingleValueConstraint(a.getAssignedValue()));
		} else {
			
		}
	}

	private void addConstraint(String var, Constraint value) {
		if(logger.isDebugEnabled()) logger.debug("Adding constraint. Var: {} Value: {}", var, value);
		this.currentConstraints.put(var, value);
		if(logger.isDebugEnabled()) logger.debug("Current constraints are now: {}", this.currentConstraints);
	}

	private void pushStacks(ASTNodeContainer node) {
		
		if(CFGUtil.isIfStart(node)){
			if(logger.isDebugEnabled()) logger.debug("Pushing {}", node);
			List<CFGEdge> edges = this.cfg.outgoingEdges(node);
			ASTNodeContainer thenNode = null;
			ASTNodeContainer elseNode = null;
			for(CFGEdge e : edges){
				if(e.getLabel().equals(CFGEdge.TRUE_LABEL)) thenNode = (ASTNodeContainer) e.getDestination();
				if(e.getLabel().equals(CFGEdge.FALSE_LABEL)) elseNode = (ASTNodeContainer) e.getDestination();
			} 
			this.conditionsStack.push(new IfStackObject(node,
					thenNode, elseNode));
			this.currentBranchStack.push(this.currentBranch);
		}
		
		if(CFGUtil.isSwitchStart(node)){
			if(logger.isDebugEnabled()) logger.debug("Pushing {}", node);
			this.conditionsStack.push(new SwitchStackObject(node,
					this.cfg.outgoingEdges(node)));
			// Save branch state
			this.currentBranchStack.push(this.currentBranch);
		}
		
	}

	private void popStacks(ASTNodeContainer node) {
		if(!this.conditionsStack.isEmpty() && 
				!this.conditionsStack.peek().getCondition()
				.getASTNode().getParent().getNodes().contains(node.getASTNode())){
			if(logger.isDebugEnabled()) logger.debug("Popping {}", this.conditionsStack.peek().getCondition());
			this.conditionsStack.pop();
			// Restore branch information
			this.currentBranch = this.currentBranchStack.pop();
		}
		// TODO join if a variable was written more than one time inside the if: check for
		// reaching definitions
	}
	
}
