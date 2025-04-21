package tools.safepatch.conditions;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.Map;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.actions.LeavesClassifier;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Status;

import ast.ASTNode;
import ast.expressions.AndExpression;
import ast.expressions.Identifier;
import ast.statements.Condition;
import jdk.net.NetworkPermission;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.rw.ReadWriteTable;

/**
 * @author Eric Camellini
 *
 */
public class ConditionDiff {

	
	private static final Logger logger = LogManager.getLogger();
	
	/*
	 * TODO
	 * - Implement basic stuff (e.g. A -> B) using https://github.com/bastikr/boolean.py
	 * - Think about how to use Z3 and/or https://github.com/angr/claripy/blob/master/claripy/vsa/strided_interval.py
	 */
	private Condition originalConditon;
	private Condition newCondition;
	
	
	public Condition getOriginalCondition() {
		return originalConditon;
	}

	public Condition getNewCondition() {
		return newCondition;
	}

	private BooleanDiff booleanAndArithmeticDiff;

	private ASTDiff diff;

	private Map<String, String> typesMap = null;
	
	private boolean bvEnabled = true;
	
	public void setBVEnabled(boolean value){
		this.bvEnabled = value;
	}
	
	public ConditionDiff(Condition originalConditon, Condition newCondition
			, ASTDiff diff) {
		this.originalConditon = originalConditon;
		this.newCondition = newCondition;
	
		this.diff = diff;
		if(originalConditon != null &&
				newCondition != null){
					if(!diff.getMappedNode(originalConditon).equals(newCondition))
						throw new RuntimeException("The two conditions to be differenced must have "
								+ "been mapped together by the ASTDiff");
		}
	}

	

//private BooleanDiff booleanDiff;
	
//	public BooleanDiff getBooleanDiff(){
//		if(this.booleanDiff == null)
//			this.generateBooleanDiff();
//		return this.booleanDiff;
//	}
//
//	private void generateBooleanDiff() {
//		//Converting original and new conditions to Z3 BoolExpr objects
//		Context ctx = new Context();
//		ConditionToZ3Converter conv = new ConditionToZ3Converter(ctx);
//		BoolExpr originalExpr = conv.getZ3BoolExpr(this.originalConditon);
//		BoolExpr newExpr = conv.getZ3BoolExpr(this.newCondition);
//		this.booleanDiff = new BooleanDiff(originalExpr, newExpr, ctx);
//	}

	public void setTypesMap(Map<String, String> typesMap) {
		this.typesMap = typesMap;
	}

	public BooleanDiff getBooleanAndArithmeticDiff(){
		if(this.booleanAndArithmeticDiff == null)
			this.generateBooleanAndArithmeticDiff();
		return this.booleanAndArithmeticDiff;
	}

	private void generateBooleanAndArithmeticDiff() {
		//Converting original and new conditions to Z3 BoolExpr objects
		Context ctx = new Context();
		ConditionToZ3Converter conv = new ConditionToZ3Converter(ctx);
		conv.setBoolOnly(false);
		conv.setBVEnabled(this.bvEnabled);
		BoolExpr originalExpr = conv.getZ3BoolExpr(this.originalConditon);
		BoolExpr newExpr = conv.getZ3BoolExpr(this.newCondition);
		this.booleanAndArithmeticDiff = new BooleanDiff(originalExpr, newExpr, ctx, conv.getCodestringExprMap());
		this.booleanAndArithmeticDiff.setTypesMap(typesMap);
	}
	
	@Override
	public String toString() {
		return "Original: " + originalConditon.toString() + "New: " + newCondition.toString();
	}
	/*
	 * This was an old version of the newImpliesOld method,
	 * where I tried to use a sort of not algorithm on the AST
	 * to check if OLDCONDITION is sorrounded by an and operator.
	 */
//	public boolean sorroundedByAnd(){
//	/*
//	 * TODO:
//	 * - Make this generic (that can work also for OR and other stuff)
//	 * - Instead of this "algorithm", perform this check using a boolean simplifier
//	 * 
//	 */
//	
//	ASTDiff astDiff = diff.getFineGrainedDiff();
//	
//	//There is an inserted AND condition as most external operator
//	if(!(newCondition.getChild(0) instanceof AndExpression)) 
//		return false;
//
//	AndExpression and = (AndExpression) newCondition.getChild(0);
//	if(!astDiff.isInserted(and))
//		return false;
//	
//	ASTNode newLeftTree = and.getChild(0);
//	ASTNode newRightTree = and.getChild(1);
//	
//	/*First, we have to check if the condition was made of a single Identifier. In
//	 * this case Gumtree doesn't perform the correct mapping and we have to perform
//	 * this algorithm in a different way. 
//	 */
//	if (originalConditon.getChildCount() == 1 && 
//			originalConditon.getChild(0) instanceof Identifier &&
//			astDiff.isDeleted(originalConditon.getChild(0))){
//		Identifier originalId = (Identifier) originalConditon.getChild(0);
//		
//		//Since Gumtree didn't perform the mapping we check the code string.
//		//This will identify modifications such as A that becomes A && B
//		if(newLeftTree.getEscapedCodeStr().equals(originalId.getEscapedCodeStr()) ||
//				newRightTree.getEscapedCodeStr().equals(originalId.getEscapedCodeStr()))
//			return true;
//		
////		IDEA TO HANDLE STUFF LIKE A && B && C --> B
////		if(newLeftTree.getChild(0) instanceof AndExpression){
////			Condition c = new Condition();
////			c.addChild(newLeftTree.getChild(0));
////			boolean b =  new ConditionDiff(originalConditon, c, diff).sorroundedByAnd();
////			if(b)
////				return true;
////		}
////		
////		if(newRightTree.getChild(0) instanceof AndExpression){
////			Condition c = new Condition();
////			c.addChild(newRightTree.getChild(0));
////			boolean b = new ConditionDiff(originalConditon, c, diff).sorroundedByAnd();
////			if(b)
////				return true;
////		}
//		
//	}
//	//else:
//	
//	//All the old leaves (identifiers) are moved
//	if(!originalConditon.getLeaves().stream().allMatch(
//			leaf -> astDiff.isMoved(leaf)))
//		return false;
//	
//	ArrayList<ASTNode> originalMovedNodes = 
//			(ArrayList<ASTNode>) originalConditon.getNodes()
//			.stream().filter(n -> astDiff.isMoved(n))
//			.collect(Collectors.toList());
//	
//	//The inserted AND has all the old
//	//part of the condition (marked as moved) on one side of its subtree
//	ArrayList<ASTNode> moveDestinations = new ArrayList<ASTNode>();
//	originalMovedNodes.forEach(n -> moveDestinations
//			.add(astDiff.getMappedDstNode(n)));
//	
//	if(!(moveDestinations.stream()
//			.allMatch(n -> newLeftTree.getNodes().contains(n)) ||
//			moveDestinations.stream()
//			.allMatch(n -> newRightTree.getNodes().contains(n))))
//		return false;
//	
//	//The operator code of every parent of the moved
//	//leaves is the same
//	for(ASTNode destLeaf : moveDestinations){
//		if(!destLeaf.isLeaf())
//			continue;
//		
//		ASTNode srcLeaf = astDiff.getMappedSrcNode(destLeaf);
//		ASTNode destParent = destLeaf.getParent();
//		ASTNode srcParent = srcLeaf.getParent();
//		while(!destParent.equals(and)){
//			if(!srcParent.getEscapedCodeStr().equals(destParent.getEscapedCodeStr()))
//				return false;
//				
//			srcParent = srcParent.getParent();
//			destParent = destParent.getParent();
//		}
//	}
//	
//	
//	return true;
//}
}
