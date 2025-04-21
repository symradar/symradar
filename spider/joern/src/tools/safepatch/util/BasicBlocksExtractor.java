package tools.safepatch.util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.gen.php.PhpParser.negateOrCast_return;

import ast.ASTNode;
import ast.expressions.CallExpression;
import ast.functionDef.Parameter;
import ast.statements.BreakStatement;
import ast.statements.CompoundStatement;
import ast.statements.Condition;
import ast.statements.ContinueStatement;
import ast.statements.ElseStatement;
import ast.statements.ExpressionStatement;
import ast.statements.GotoStatement;
import ast.statements.IfStatement;
import ast.statements.Label;
import ast.statements.ReturnStatement;
import ast.statements.WhileStatement;
import cdg.CDG;
import cdg.DominatorTree;
import cfg.CFG;
import cfg.CFGEdge;
import cfg.C.CCFG;
import tools.safepatch.cfgimpl.CFGIterator;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import jdk.nashorn.internal.ir.BreakableNode;
public class BasicBlocksExtractor {
	
	private static final Logger logger = LogManager.getLogger();
	private static final String NOT_AN_AST_CONTAINER_WARN = "Exploring a node that is not an ASTNodeContainer: ";
	
	private boolean heuristicsEnabled = true;
	/**
	 * If the heuristicsEnabled attribute is true the heuristics to improve the basic 
	 * blocks identification are enabled.
	 * Heuristics use information from the content of the CFG node such as
	 * what kind of statement it is (e.g. exit(), otherwise not handled)
	 * and add consecutive statements to a new basic block if they cannot be added
	 * to any other block.
	 */
	public void setHeuristicsEnabled(boolean value){
		this.heuristicsEnabled = value;
	}
	
	/**
	 * Given a Joern CFG and corresponding CDG,
	 * this function returns the basic blocks.
	 */
	public List<BasicBlock> getBasicBlocks(CCFG cfg, CDG cdg){
		return getBasicBlocks(cfg, getBasicBlockLeaders(cfg, cdg));
	}
	
	/**
	 * Given a Joern CFG and corresponding CDG,
	 * this function returns the basic block leaders.
	 */
	private Set<ASTNodeContainer> getBasicBlockLeaders(CCFG cfg, CDG cdg){
		if(logger.isDebugEnabled()) logger.debug("Extracting basic block leaders...");
		CFGIterator walker = new CFGIterator(cfg);
		DominatorTree<CFGNode> dominatorTree = cdg.getDominatorTree();
		
		skipParameters(walker);
		Set<ASTNodeContainer> leaders = new HashSet<ASTNodeContainer>();
		
		CFGNode current = walker.getCurrentNode();
		if(current == null){
			if(logger.isDebugEnabled()) logger.debug("No leaders found: function is empty.");
			return leaders;
		}
		
		//The first node is a leader
		if(current instanceof ASTNodeContainer){
			if(((ASTNodeContainer) current).getASTNode() instanceof Parameter){
				if(logger.isDebugEnabled()) logger.debug("No leaders found: function is empty.");
				return leaders;
			}
			if(logger.isDebugEnabled()) logger.debug("Adding leader (first node): {}", current);
			leaders.add((ASTNodeContainer) current);
		} else if(logger.isWarnEnabled()) logger.warn(NOT_AN_AST_CONTAINER_WARN + current);
		
		while(current != null){
			if(logger.isDebugEnabled()) logger.debug("Current node: {}", current);
			
			if(current instanceof ASTNodeContainer){
				// We check if the node node has more than one outgoing edge. If yes, we 
				// mark the destination nodes as block leaders.
				if(cfg.outDegree(current) > 1){
					
					for(CFGEdge edge : cfg.outgoingEdges(current)){
						if(edge.getDestination() instanceof ASTNodeContainer){
							if(logger.isDebugEnabled()) logger.debug("Adding leader: {}", edge.getDestination());
							leaders.add((ASTNodeContainer) edge.getDestination());
						}
						else if(logger.isWarnEnabled()) logger.warn(NOT_AN_AST_CONTAINER_WARN + edge.getDestination());
					}
					
					//We also add the dominator as a block leader.
					CFGNode dominator = dominatorTree.getDominator(current);
					if(dominator instanceof ASTNodeContainer){
						if(dominator != null){
							if(logger.isDebugEnabled()) logger.debug("Adding leader: {}", dominator);
							leaders.add((ASTNodeContainer) dominator);
						} if(logger.isWarnEnabled()) logger.warn("Null dominator for node " + current);
					}
					else if(logger.isWarnEnabled()) logger.warn(NOT_AN_AST_CONTAINER_WARN + dominator);
				}
				
				//And also the labels:
				if(((ASTNodeContainer) current).getASTNode() instanceof Label){
					if(logger.isDebugEnabled()) logger.debug("Adding leader: {}", current);
					leaders.add((ASTNodeContainer) current);
				}
				
			} else if(logger.isWarnEnabled()) logger.warn(NOT_AN_AST_CONTAINER_WARN + current);
			
			if(walker.hasNext()){
				walker.moveToNext();
				current = walker.getCurrentNode();
			} else current = null;
		}
		
		if(logger.isDebugEnabled()) logger.debug("Leaders: {}", leaders);
		return leaders;
	}

	/**
	 * Given a Joern CFG a set of basic block leaders
	 * this function returns the set of basic blocks.
	 */
	private List<BasicBlock> getBasicBlocks(CCFG cfg, Set<ASTNodeContainer> leaders){
		
		if(logger.isDebugEnabled()) logger.debug("Extracting basic blocks...");
		List<BasicBlock> basicBlocks = new ArrayList<BasicBlock>();
		
		CFGIterator walker = new CFGIterator(cfg);
		skipParameters(walker);
		
		CFGNode current = walker.getCurrentNode();
		BasicBlock currentBlock = null;
		
		if(leaders.size() == 0 || current == null ||
				(current instanceof ASTNodeContainer && ((ASTNodeContainer) current).getASTNode() instanceof Parameter)){
			if(logger.isDebugEnabled()) logger.debug("No basic blocks found, function body is empty");
			return basicBlocks;
		}
		
		while(current != null){
			if(logger.isDebugEnabled()) logger.debug("Current node: {}", current);
			
			if(current instanceof ASTNodeContainer){
				
				//Block starts
				if(leaders.contains(current)){
					if(logger.isDebugEnabled()) logger.debug("Leader found: {}", current);
					//The block that we were creating is complete, we add it to the
					//basic blocks and create a new one
					if(currentBlock != null){
						if(logger.isDebugEnabled()) logger.debug("Finalizing block: {}", currentBlock);
						basicBlocks.add(currentBlock);
					} else if(logger.isWarnEnabled()) logger.warn("Not finalizing current block because null.");
					
					if(logger.isDebugEnabled()) logger.debug("Creating new block.");
					currentBlock = new BasicBlock();
				} else if(heuristicsEnabled && currentBlock == null){
					//If the current block is null and the current node is not a 
					//leader it means that the block of the current node cannot be
					//determined. The heuristic adds consecutive nodes that cannot
					//be assigned to any block to a new basic block.
					currentBlock = new BasicBlock();
					if(logger.isDebugEnabled()) logger.debug("Heuristic: creating new basic block for sequence of nodes for which the block cannot be found.");
				}
				
				//We add the current node to the current basic block
				if(currentBlock != null){
					if(logger.isDebugEnabled()) logger.debug("Adding to current block: {}", current);
					currentBlock.addNode((ASTNodeContainer) current);

					//We check for special known cases
					if(heuristicsEnabled){
						ASTNode currentAstNode = ((ASTNodeContainer) current).getASTNode();
						if(currentAstNode instanceof GotoStatement ||
								currentAstNode instanceof BreakStatement ||
								currentAstNode instanceof ContinueStatement ||
								currentAstNode instanceof ReturnStatement ||
								(currentAstNode instanceof ExpressionStatement &&
										currentAstNode.getChild(0) instanceof CallExpression &&
										nonReturningFunctions.stream().anyMatch(f -> f.equals(currentAstNode.getChild(0).getChild(0).getEscapedCodeStr())))
								){
							if(logger.isDebugEnabled()) logger.debug("Finalizing block: {}", currentBlock);
							basicBlocks.add(currentBlock);
							currentBlock = null;
						}
					}
					
				} else if(logger.isWarnEnabled()) logger.warn("Node skipped because current block is null: {}", current);
				
			} else if(logger.isWarnEnabled()) logger.warn(NOT_AN_AST_CONTAINER_WARN + current);
			
			
			if(walker.hasNext()){
				walker.moveToNext();
				current = walker.getCurrentNode();
			} else current = null;
		}
		
		// We still miss the last block, it must be finalized
		if(currentBlock != null){
			//If the current block is not null it means that we just exited from the while, in some way
			if(logger.isDebugEnabled()) logger.debug("Finalizing LAST block: {}", currentBlock);
			basicBlocks.add(currentBlock);
		}
		
		if(logger.isDebugEnabled()) logger.debug("Basic blocks found: {}", basicBlocks);
		return basicBlocks;
	}
	
	/**
	 * Skips the function parameters and moved the walker pointer to the first node of
	 * the function body. If the function body is empty, the pointer will point to the last
	 * parameter.
	 */
	private void skipParameters(CFGIterator walker) {
		//Skipping parameters:
		while(walker.getCurrentNode() != null &&
				walker.getCurrentNode() instanceof ASTNodeContainer &&
				((ASTNodeContainer) walker.getCurrentNode()).getASTNode() instanceof Parameter){
			if(walker.hasNext()) walker.moveToNext();
			else break;
		}
	}
	
	private static final String[] nonReturningFunctionsArray = new String[] { "exit", "longjmp"};
	private static final List<String> nonReturningFunctions = Arrays.asList(nonReturningFunctionsArray);
	
}
