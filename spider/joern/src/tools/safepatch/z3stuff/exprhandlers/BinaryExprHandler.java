/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.expressions.AndExpression;
import ast.expressions.BinaryExpression;
import ast.expressions.OrExpression;

import com.microsoft.z3.Context;

/**
 * Convert a binary expression 
 * op1 + op2 into a z3 expression.
 * @author machiry
 *
 */
public class BinaryExprHandler extends ASTToZ3ExprHandler {
	

	public BinaryExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}

	/* (non-Javadoc)
	 * @see tools.safepatch.z3stuff.exprhandlers.ASTToZ3ExprHandler#processASTNode(java.lang.String)
	 */
	@Override
	public Object processASTNode(String targetSym) {
		BinaryExpression targetExpr = (BinaryExpression)this.targetNode;
		String opCode = targetExpr.getOperatorCode();
		Object leftOp = null;
		Object rightOp = null;
		if(targetExpr instanceof AndExpression) {
			opCode = "&&";
		}
		if(targetExpr instanceof OrExpression) {
			opCode = "||";
		}
		try {
			// get left operand.
			leftOp = this.handleChildASTNode(this.targetNode.getChild(0), targetSym);
			// get right operand.
			rightOp = this.handleChildASTNode(this.targetNode.getChild(1), targetSym);
			// perform the binary op.
			Object retVal = Z3Utils.processBinOp(this.targetCtx, leftOp, rightOp, opCode);
			
			if(targetSym != null && this.statementNode != this.targetNode) {
				boolean leftPresent, rightPresent;
				
				leftPresent = this.isSymbolInTheNode(this.targetNode.getChild(0), targetSym);
				rightPresent = this.isSymbolInTheNode(this.targetNode.getChild(1), targetSym);
				if(leftPresent && !rightPresent) {
					retVal = leftOp;
				}
				if(!leftPresent && rightPresent) {
					retVal = rightOp;
				}
			}
			return retVal;
		} catch(Exception e) {
			logger.debug("Exception occured while trying to generate z3 symbol for {}", this.targetNode.getEscapedCodeStr());
			Object retVal = this.targetConv.generateStringSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
			return retVal;
		}
	}

}
