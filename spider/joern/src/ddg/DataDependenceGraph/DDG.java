package ddg.DataDependenceGraph;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import ast.ASTNode;

public class DDG
{

	private Set<DefUseRelation> defUseEdges = new HashSet<DefUseRelation>();

	public Set<DefUseRelation> getDefUseEdges()
	{
		return defUseEdges;
	}

	public void add(Object srcId, Object dstId, String symbol)
	{
		DefUseRelation statementPair = new DefUseRelation(srcId, dstId, symbol);
		defUseEdges.add(statementPair);
	};

	/**
	 * Compares the DDG with another DDG and returns a DDGDifference object
	 * telling us which edges need to be added/removed to transform one DDG into
	 * the other.
	 * 
	 * @param other
	 * @return
	 */
	public DDGDifference difference(DDG other)
	{
		DDGDifference retval = new DDGDifference();
		List<DefUseRelation> thisEdges = new LinkedList<DefUseRelation>(
				this.getDefUseEdges());

		Set<DefUseRelation> otherEdges = new HashSet<DefUseRelation>(
				other.getDefUseEdges());

		while (thisEdges.size() > 0)
		{
			DefUseRelation elem = thisEdges.remove(0);
			if (otherEdges.contains(elem))
				otherEdges.remove(elem);
			else
				retval.addRelToRemove(elem);
		}

		for (DefUseRelation elem : otherEdges)
			retval.addRelToAdd(elem);

		return retval;
	}
	
	/**
	 * Given a statement and a variable that the statement uses,
	 * this method returns all the definitions of that variable
	 * that can reach this statement (a definition is a statement
	 * that defines a variable)
	 */
	public List<ASTNode> getReachingDefs(ASTNode node, String use){
		List<ASTNode> retval = new ArrayList<ASTNode>();
		for(DefUseRelation rel : this.defUseEdges){
			if(rel.dst.equals(node) && rel.symbol.equals(use))
				retval.add((ASTNode) rel.src);
		}
		return retval;
	}
	
	/***
	 *  Get all the ASTNode which could used the value defined by this statement.
	 *  
	 * @param node Src Node.
	 * @param def Target symbol defined by this Node.
	 * @return List of Nodes which use the symbol defined by the given src node.
	 */
	public List<ASTNode> getReachingUses(ASTNode node, String def) {
		List<ASTNode> retval = new ArrayList<ASTNode>();
		for(DefUseRelation rel: this.defUseEdges) {
			if(rel.src.equals(node) && rel.symbol.equals(def)) {
				if(rel.dst instanceof ASTNode) {
					retval.add((ASTNode) rel.dst);
				}
			}
		}
		return retval;
	}
}
