/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;


import java.util.Map;

import com.microsoft.z3.Context;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.expressions.AssignmentExpr;

/**
 * This converts the assignment expression into z3 object.
 * i.e., a = b into z3 object.
 * @author machiry
 *
 */
public class AssignmentExprHandler extends ASTToZ3ExprHandler {

	public AssignmentExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}
	
	@Override
	public Object processASTNode(String targetSym) {
		// here we just return the z3 object of the right hand side expression.
		AssignmentExpr targetExpr = (AssignmentExpr)this.targetNode;
		
		ASTNode leftChild = targetExpr.getLeft();
		String operatorCode = this.targetNode.getOperatorCode();
				
		ASTNode rightChild = targetExpr.getRight();
		
		Object rightObj = this.handleChildASTNode(rightChild, targetSym);
		Object targetRet = rightObj;
		// see if we are doing +=, -= or other shortcut operators.
		if(!operatorCode.equals("=") && operatorCode.endsWith("=")) {
			int endIdx = operatorCode.length() - 1;
			String inlineOp = operatorCode.substring(0, endIdx);
			Object leftObj = this.getExistingVal(leftChild.getEscapedCodeStr());
			 targetRet = Z3Utils.processBinOp(this.targetCtx, leftObj, rightObj, inlineOp);
		}
		return targetRet;
		
	}

}
