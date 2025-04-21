/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

import com.microsoft.z3.ArithExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.expressions.AndExpression;
import ast.expressions.BinaryExpression;
import ast.expressions.CallExpression;
import ast.expressions.CastExpression;
import ast.expressions.OrExpression;
import ast.expressions.UnaryOp;
import ast.expressions.UnaryOperator;
import ast.statements.Condition;

/**
 * @author machiry
 *
 */
public class CondExprHandler extends ASTToZ3ExprHandler {


	private static final String LIKELY_MACRO = "likely";
	private static final String UNLIKELY_MACRO = "unlikely";
	private static final String C_NOT = "!";
	private static final String[] handlableBinOps = {">", "<", ">=", "<=", "==", "!="};
	
	public CondExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}
	
	
	public Object processConditionalChild(ASTNode childNode, String targetSym) {
		CondExprHandler currHandler = new CondExprHandler(this.targetGraphs, childNode, this.statementNode,this.requiredValues, this.targetCtx, this.targetConv);
		return currHandler.processASTNode(targetSym);
	}

	/* (non-Javadoc)
	 * @see tools.safepatch.z3stuff.IASTZ3ExprHandler#processASTNode()
	 */
	@Override
	public Object processASTNode(String targetSym) {
		// process the child
		Context ctx = this.targetCtx;
		ASTNode conditionContent = this.targetNode.getChild(0);
		Object childOb = this.handleChildASTNode(conditionContent, targetSym);
		// if this is not a boolean expression?
		if(childOb instanceof BoolExpr) {
			return childOb;
		}
		if(targetSym == null) {
			//add a constraint that the result should not be 0.
			return ctx.mkNot(ctx.mkEq((Expr)childOb, (Expr)Z3Utils.generateConstant(ctx, 0)));
		} else {
			// this is when there is a embedded expression inside a condition.
			// i.e, (dp++ == ZEP)
			// we might want the value of dp.
			return childOb;
		}
		
		/*if(this.targetNode instanceof Condition) {
			Condition condition = (Condition)this.targetNode;
			
			
			// remove likely and unlikely macros.
			if(condition.getChild(0) instanceof CallExpression &&
					(condition.getChild(0).getChild(0).getEscapedCodeStr().equals(LIKELY_MACRO) ||
							condition.getChild(0).getChild(0).getEscapedCodeStr().equals(UNLIKELY_MACRO))){
				conditionContent = condition.getChild(0).getChild(1).getChild(0).getChild(0);
			} else {
				conditionContent = condition.getChild(0);
			}
			return this.processConditionalChild(conditionContent, targetSym);
		} else {
			return this.handleNonCondition(conditionContent, targetSym);
		}*/
		
	}
	
	public Object handleNonCondition(ASTNode conditionContent, String targetSym) {
		Context ctx = this.targetCtx;
		/*if(conditionContent instanceof CastExpression){
			return this.processConditionalChild(conditionContent.getChild(1), targetSym);
		}
		
		if(conditionContent instanceof AndExpression){
			Object child1 = this.processConditionalChild(conditionContent.getChild(0), targetSym);
			Object child2 = this.processConditionalChild(conditionContent.getChild(1), targetSym);
			return ctx.mkAnd((BoolExpr)child1, (BoolExpr)child2);
		}

		if(conditionContent instanceof OrExpression){
			Object child1 = this.processConditionalChild(conditionContent.getChild(0), targetSym);
			Object child2 = this.processConditionalChild(conditionContent.getChild(1), targetSym);
			return ctx.mkOr((BoolExpr)child1, (BoolExpr)child2);
		}

		if(conditionContent instanceof UnaryOp &&
				conditionContent.getChild(0) instanceof UnaryOperator &&
				conditionContent.getChild(0).getEscapedCodeStr().equals(C_NOT)){
			Object childOb = this.handleChildASTNode(conditionContent.getChild(1), targetSym);
			if(childOb instanceof BoolExpr) {
				return ctx.mkNot((BoolExpr)childOb);
			} else {
				return ctx.mkNot(ctx.mkEq((Expr)childOb, (Expr)Z3Utils.generateConstant(ctx, 0)));
			}
		}
		
		if(conditionContent instanceof BinaryExpression) {
			String opCode = conditionContent.getOperatorCode();
			List<String> tmpSt = Arrays.asList(CondExprHandler.handlableBinOps);
			if(tmpSt.contains(opCode)) {
				return this.handleChildASTNode(conditionContent, targetSym);
			}
		}*/
		
		
		// this is the case when we have if(cond)..
		Object childOb = this.handleChildASTNode(conditionContent, targetSym);
		if(childOb instanceof BoolExpr) {
			return childOb;
		}
		return ctx.mkNot(ctx.mkEq((Expr)childOb, (Expr)Z3Utils.generateConstant(ctx, 0)));
	}

}
