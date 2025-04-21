package tools.safepatch.fgdiff;

import java.nio.file.attribute.AclEntryFlag;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.actions.model.Action;
import com.github.gumtreediff.actions.model.Delete;
import com.github.gumtreediff.actions.model.Insert;
import com.github.gumtreediff.actions.model.Move;
import com.github.gumtreediff.actions.model.Update;
import com.github.gumtreediff.matchers.MappingStore;
import com.github.gumtreediff.tree.ITree;

import ast.ASTNode;
import ast.expressions.AssignmentExpr;

/**
 * @author Eric Camellini
 * 
 * Fine-grained AST diff (Obtained using Gumtree) 
 */
public class ASTDiff {
	
	public enum ACTION_TYPE {
        INSERT, 
        DELETE, 
        MOVE,
        UPDATE
    }

	private static final Logger logger = LogManager.getLogger();
	
	private MappingStore mappings;
	private GumtreeASTMapper mapper;

	private List<Action> gumtreeActions;
	
	private Map<ASTNode, ACTION_TYPE> originalActionByNode; 
	private Map<ASTNode, ACTION_TYPE> newActionByNode;

	public List<Action> getGumtreeActions() {
		return gumtreeActions;
	}

	public MappingStore getMappings() {
		return mappings;
	}


	public List<ASTNode> getNewNodesByActionType(ACTION_TYPE t){
		return new ArrayList<ASTNode>(this.newActionByNode.entrySet()
				.stream()
				.filter(e -> e.getValue().equals(t))
				.collect(Collectors.toMap(p -> p.getKey(), p -> p.getValue())).keySet());
	}
	
	public List<ASTNode> getOriginalNodesByActionType(ACTION_TYPE t){
		return new ArrayList<ASTNode>(this.originalActionByNode.entrySet()
				.stream()
				.filter(e -> e.getValue().equals(t))
				.collect(Collectors.toMap(p -> p.getKey(), p -> p.getValue())).keySet());
	}

	public ASTDiff(MappingStore mappings, List<Action> actions, GumtreeASTMapper mapper) {
		this.mappings = mappings;
		this.gumtreeActions = actions;
		this.mapper = mapper;
		this.newActionByNode = new HashMap<>();
		this.originalActionByNode = new HashMap<>();
		for(Action a : this.gumtreeActions){
			this.mapAction(a);
		}
	}
	
	
	
	public ACTION_TYPE getActionByNode(ASTNode n){
		ACTION_TYPE oldA = this.originalActionByNode.get(n);
		ACTION_TYPE newA = this.newActionByNode.get(n);
		if(oldA != null && newA != null)
			throw new RuntimeException("ASTDiff contains the same node marked both as old and new");
		else if (oldA != null)
			return oldA;
		else if (newA != null)
			return newA;
		else
			return null; //No actions associated to this node.
	}

	public boolean isInserted(ASTNode n){
		return this.getActionByNode(n) == ACTION_TYPE.INSERT;
	}

	public boolean isMoved(ASTNode n){
		return this.getActionByNode(n) == ACTION_TYPE.MOVE;
	}

	public boolean isDeleted(ASTNode n){
		return this.getActionByNode(n) == ACTION_TYPE.DELETE;
	}

	public boolean isUpdated(ASTNode n){
		return this.getActionByNode(n) == ACTION_TYPE.UPDATE;
	}
	
	/**
	 * Returns from the original CFG all the nodes that are
	 * marked as updated, deleted, moved or inserted.
	 */
	public List<ASTNode> getOriginalNodesByAnyActionType(){
		List<ASTNode> nodes = new ArrayList<ASTNode>();
		nodes.addAll(getOriginalNodesByActionType(ACTION_TYPE.INSERT));
		nodes.addAll(getOriginalNodesByActionType(ACTION_TYPE.DELETE));
		nodes.addAll(getOriginalNodesByActionType(ACTION_TYPE.UPDATE));
		nodes.addAll(getOriginalNodesByActionType(ACTION_TYPE.MOVE));
		return nodes;
	}
	
	/**
	 * Returns from the new CFG all the nodes that are
	 * marked as updated, deleted, moved or inserted.
	 */
	public List<ASTNode> getNewNodesByAnyActionType(){
		List<ASTNode> nodes = new ArrayList<ASTNode>();
		nodes.addAll(getNewNodesByActionType(ACTION_TYPE.INSERT));
		nodes.addAll(getNewNodesByActionType(ACTION_TYPE.DELETE));
		nodes.addAll(getNewNodesByActionType(ACTION_TYPE.UPDATE));
		nodes.addAll(getNewNodesByActionType(ACTION_TYPE.MOVE));
		return nodes;
	}
	/**
	 * Returns the node that Gumtree decided to map to n.
	 * If n is in the new AST then the output will be in the
	 * old one and vice-versa.
	 * If there is no mapping for n then null is returned.
	 */
	public ASTNode getMappedNode(ASTNode n){
		ASTNode src = getMappedSrcNode(n);
		if(src != null) return src;
		
		ASTNode dst = getMappedDstNode(n);
		if(dst != null) return dst;
		
		return null;
	}
	
	/**
	 * Returns the node that Gumtree decided to map as source of n.
	 * n should be a node from the new AST.
	 */
	public ASTNode getMappedSrcNode(ASTNode n){
		ITree itree = this.mapper.getMappedITree(n);
		ITree mapped = this.mappings.getSrc(itree);
		if (mapped != null)
			return this.mapper.getMappedASTNode(mapped);		
		return null;
	}
	
	/**
	 * Returns the node that Gumtree decided to map as destination of n.
	 * n should be a node from the original AST.
	 */
	public ASTNode getMappedDstNode(ASTNode n){
		ITree itree = this.mapper.getMappedITree(n);
		ITree mapped = this.mappings.getDst(itree);
		if (mapped != null)
			return this.mapper.getMappedASTNode(mapped);		
		return null;
	}
	
	/**
	 * Given a node, returns true if any of its sub-nodes (descendants together
	 * with the node itself) is marked as inserted, deleted, updated or moved.
	 */
	public boolean containsSomeActions(ASTNode node){
		return node.getNodes().stream().anyMatch(n -> (isDeleted(n) ||
				isInserted(n) || isUpdated(n)) || isMoved(n));
	}
	
	/**
	 * Given a node, returns true if some of its sub-nodes (descendants together
	 * with the node itself) are changed according to the ACTION_TYPE type.
	 */
	public boolean containsSomeActionsOfType(ASTNode node, ACTION_TYPE type){
		return node.getNodes().stream().anyMatch(
				n -> 
				(type == ACTION_TYPE.INSERT ? isInserted(n) : 
					(type == ACTION_TYPE.DELETE ? isDeleted(n) : 
						(type == ACTION_TYPE.MOVE ? isMoved(n) : isUpdated(n)))));
	}
	
	/**
	 * Given a node, returns true if SOME of its sub-nodes (descendants together
	 * with the node itself) are changed, and all these changes are of ACTION_TYPE type.
	 */
	public boolean containsOnlyActionsOfType(ASTNode node, ACTION_TYPE type){
		return containsSomeActions(node) &&
				node.getNodes().stream().allMatch(
				n -> 
				(getActionByNode(n) == null || 
				(type == ACTION_TYPE.INSERT ? isInserted(n) : 
					(type == ACTION_TYPE.DELETE ? isDeleted(n) : 
						(type == ACTION_TYPE.MOVE ? isMoved(n) : isUpdated(n))))));
	}
	
	/**
	 * Given a node, returns true if ALL of its sub-nodes (descendants together
	 * with the node itself) are changed, and all these changes are of ACTION_TYPE type.
	 */
	public boolean areAllNodesMarkedAs(ASTNode node, ACTION_TYPE type){
		return node.getNodes().stream().allMatch(
				n -> 
				(type == ACTION_TYPE.INSERT ? isInserted(n) : 
					(type == ACTION_TYPE.DELETE ? isDeleted(n) : 
						(type == ACTION_TYPE.MOVE ? isMoved(n) : isUpdated(n)))));
	}
	
	/**
	 * Maps the Gumtree Action a on the original and new Joern ASTs.
	 */
	private void mapAction(Action a){
		ITree actionNode = a.getNode();
		ASTNode node = this.mapper.getMappedASTNode(actionNode);
		//Necessary for MOVE and UPDATE actions: the actionNode in these
		//is always the one in the old tree, so i get the corresponding
		//one in the new. It will be null in case the action is an INSERT/UPDATE
		ASTNode mappedNode = this.getMappedDstNode(node);
		
		if(a instanceof Insert){
			//If the action is an insert then I map it on the corresponding
			//node in the new AST.
			//Since only leaves can be inserted, i don't map as any descendant.
			this.newActionByNode.put(node, ACTION_TYPE.INSERT);
		}
		if(a instanceof Delete){
			//If the action is a delete then I map it on the corresponding
			//node in the old AST.
			//Since only leaves can be deleted, i don't map as any descendant.
			this.originalActionByNode.put(node, ACTION_TYPE.DELETE);
		}
		if(a instanceof Update){
			//If the action is an update I map it both on the old and on the new AST.
			//Since updates affect only a single node's value I don't map any descendant.
			this.originalActionByNode.put(node, ACTION_TYPE.UPDATE);
			this.newActionByNode.put(mappedNode, ACTION_TYPE.UPDATE);
		}
		if(a instanceof Move){
			//If the action is a move I map it both on the old and on the new AST.
			if (mappedNode == null){
				throw new RuntimeException("MOVE or UPDATE action without mapped node.");
			}
			//Since MOVE actions can move also nodes that are not leaves, 
			//I mark as moved the nodes and all their descendants (if not already marked
			//as inserted, deleted or updated)
			node.getNodes().stream().forEach(n -> {
				if(this.originalActionByNode.get(n) == null) 
					this.originalActionByNode.put(n, ACTION_TYPE.MOVE);
			});
			mappedNode.getNodes().stream().forEach(n -> {
				if(this.newActionByNode.get(n) == null) 
					this.newActionByNode.put(n, ACTION_TYPE.MOVE);
			});
		}
	}
	
	/**
	 * Returns from the new AST the nodes that are affected by a given type of action.
	 * A node is affected by an action if the node itself is marked with that action or
	 * if any of its children is.
	 */
	public List<ASTNode> getNewAffectedNodes(Class<?> cl, ACTION_TYPE t){
		return this.getAffectedNodesHelper(cl, t, NEW_OR_ORIGINAL.NEW);
	}
	
	/**
	 * Returns from the original AST the nodes that are affected by a given type of action.
	 * A node is affected by an action if the node itself is marked with that action or
	 * if any of its children is.
	 */
	public List<ASTNode> getOriginalAffectedNodes(Class<?> cl, ACTION_TYPE t){
		return this.getAffectedNodesHelper(cl, t, NEW_OR_ORIGINAL.ORIGINAL);
	}
	
	private enum NEW_OR_ORIGINAL {
		NEW,
		ORIGINAL
	}
	
	private List<ASTNode> getAffectedNodesHelper(Class<?> cl, ACTION_TYPE t, NEW_OR_ORIGINAL newOrOld){		
		if(!ASTNode.class.isAssignableFrom(cl))
			throw new RuntimeException("Not an ASTNode subclass.");
		
		List<ASTNode> affected = new ArrayList<ASTNode>();
		Collection<ASTNode> source = newOrOld == NEW_OR_ORIGINAL.NEW ? getNewNodesByActionType(t) : getOriginalNodesByActionType(t);
		source.forEach(
				n -> {
					if(cl.isInstance(n)){
						affected.add(n);
					}

					n.getParents().stream().forEach(
							p -> {
								if(cl.isInstance(p) &&
										!affected.contains(p)){
									affected.add(p);
								}
							});
				});
		return affected;
	}
	
	/**
	 * Returns from the new AST the nodes that are affected by any kind of action.
	 * A node is affected by an action if the node itself is marked with that action or
	 * if any of its children is. The node will be marked
	 * as affected also if its old version has some children marked as deleted.
	 */
	public List<ASTNode> getNewAffectedNodes(Class<?> cl){
		return this.getAffectedNodesHelper(cl, NEW_OR_ORIGINAL.NEW);
	}
	
	/**
	 * Returns from the original AST the nodes that are affected by any kind of action.
	 * A node is affected by an action if the node itself is marked with that action or
	 * if any of its children is. The node will be marked
	 * as affected also if its new version has some children marked as inserted.
	 */
	public List<ASTNode> getOriginalAffectedNodes(Class<?> cl){
		return this.getAffectedNodesHelper(cl, NEW_OR_ORIGINAL.ORIGINAL);
	}
	
	private List<ASTNode> getAffectedNodesHelper(Class<?> cl, NEW_OR_ORIGINAL newOrOriginal){
		
		//Getting nodes affected by INSERTions (if we look in the NEW AST) or
		//DELETEs (if we look in the ORIGINAL AST)
		List<ASTNode> affectedNodes = (newOrOriginal == NEW_OR_ORIGINAL.NEW ?
				getNewAffectedNodes(cl, ACTION_TYPE.INSERT) :
					getOriginalAffectedNodes(cl, ACTION_TYPE.DELETE));

		//Getting nodes affected by UPDATEs
		if(newOrOriginal == NEW_OR_ORIGINAL.NEW){
			getNewAffectedNodes(cl, ACTION_TYPE.UPDATE).forEach(n -> {
				if(!affectedNodes.contains(n)) affectedNodes.add(n);});
		} else {
			getOriginalAffectedNodes(cl, ACTION_TYPE.UPDATE).forEach(n -> {
				if(!affectedNodes.contains(n)) affectedNodes.add(n);});
		}

		//Getting nodes affected by MOVEs
		if(newOrOriginal == NEW_OR_ORIGINAL.NEW){
			getNewAffectedNodes(cl, ACTION_TYPE.MOVE).forEach(n -> {
				if(!affectedNodes.contains(n)) affectedNodes.add(n);});
		} else {
			getOriginalAffectedNodes(cl, ACTION_TYPE.MOVE).forEach(n -> {
				if(!affectedNodes.contains(n)) affectedNodes.add(n);});
		}

		if(newOrOriginal == NEW_OR_ORIGINAL.NEW){
			//If we look for affected nodes in the NEW AST we also get the nodes that are mapped
			//to nodes in the ORIGINAL AST that have some children DELETEd
			getOriginalAffectedNodes(cl, ACTION_TYPE.DELETE).forEach(n -> {
				ASTNode mapped = getMappedDstNode(n);
				if(mapped != null && !affectedNodes.contains(mapped)) affectedNodes.add(mapped);});
		} else {
			//If we look for affected nodes in the ORIGINAL AST we also get the nodes that are mapped
			//to nodes in the NEW AST that have some children INSERTed
			getNewAffectedNodes(cl, ACTION_TYPE.INSERT).forEach(n -> {
				ASTNode mapped = getMappedSrcNode(n);
				if(mapped != null && !affectedNodes.contains(mapped)) affectedNodes.add(mapped);});
		}

		return affectedNodes;
	}

}
