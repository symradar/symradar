package cdg;

import graphutils.AbstractGraph;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ast.ASTNode;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGEntryNode;
import cfg.nodes.CFGNode;
import cdg.CDGEdge;

public class CDG extends AbstractGraph<CFGNode, CDGEdge>
{

	private DominatorTree<CFGNode> dominatorTree;
	
	private HashMap<ASTNode, ArrayList<ASTNode>> pathConstraints;
	private List<CDGEdge> targetEdges = null;

	private CDG()
	{
		this.pathConstraints = new HashMap<ASTNode, ArrayList<ASTNode>>();
	}

	public DominatorTree<CFGNode> getDominatorTree()
	{
		return this.dominatorTree;
	}

	public static CDG newInstance(DominatorTree<CFGNode> dominatorTree)
	{
		CDG cdg = new CDG();
		cdg.dominatorTree = dominatorTree;
		for (CFGNode vertex : dominatorTree.getVertices())
		{
			Set<CFGNode> frontier = dominatorTree.dominanceFrontier(vertex);
			if (frontier != null)
			{
				cdg.addVertex(vertex);
				for (CFGNode f : frontier)
				{
					cdg.addVertex(f);
					cdg.addEdge(new CDGEdge(f, vertex));
				}
			}
		}
		return cdg;
	}
	
	private List<ASTNode> getPathConstraintList(ASTNode currNode, HashSet<ASTNode> visitedNodes) {
		List<ASTNode> retVal = null;
			if(this.pathConstraints.containsKey(currNode)) {
				retVal = this.pathConstraints.get(currNode);
			}
			else {
				if(!visitedNodes.contains(currNode)) {
					ArrayList<ASTNode> newL = new ArrayList<ASTNode>();
					for(CDGEdge currEd: this.targetEdges) {
						CFGNode srcNode = currEd.getSource();
						CFGNode dstNode = currEd.getDestination();
						if(dstNode instanceof ASTNodeContainer && 
						   srcNode instanceof ASTNodeContainer) {
							
							ASTNode srcASTNode = ((ASTNodeContainer)srcNode).getASTNode();
							ASTNode dstASTNode = ((ASTNodeContainer)dstNode).getASTNode();
							
							if(dstASTNode.equals(currNode)) {
								visitedNodes.add(currNode);
								newL.add(srcASTNode);
								List<ASTNode> srcPathCon = this.getPathConstraintList(srcASTNode, visitedNodes);
								if(srcPathCon == null) {
									this.pathConstraints.put(srcASTNode, null);
								}
								if(srcPathCon != null) {
									for(ASTNode cu: srcPathCon) {
										if(!newL.contains(cu)) {
											newL.add(cu);
										}
									}
								}						
								visitedNodes.remove(currNode);
							}
						}
					}
					this.pathConstraints.put(currNode, newL);
					retVal = this.pathConstraints.get(currNode);
				}
			}
		
		return retVal;
	}
	
	
	public List<ASTNode> getPathConstraintList(ASTNode currNode) {
		this.targetEdges = this.getEdges();
		return this.getPathConstraintList(currNode, new HashSet<ASTNode>());
	}

}
