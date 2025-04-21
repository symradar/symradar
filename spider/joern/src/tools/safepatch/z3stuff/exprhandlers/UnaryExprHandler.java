/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.expressions.IncDec;
import ast.expressions.IncDecOp;
import ast.expressions.UnaryExpression;
import ast.expressions.UnaryOp;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;

/**
 * Handle unary operator like:
 *  a++
 *  !a
 * @author machiry
 *
 */
public class UnaryExprHandler extends ASTToZ3ExprHandler {

	public UnaryExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}
	
	private String getUnaryOperatorCode(UnaryExpression currNode) {
		String opCode = currNode.getOperatorCode();
		if(opCode.isEmpty()) {
			for(ASTNode currC: currNode.getChildren()) {
				if(currC instanceof IncDec) {
					opCode = ((IncDec)currC).getEscapedCodeStr().trim();
					break;
				}
			}
		}
		return opCode;
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		Object toRet = null;
		boolean isHandled = false;
		if(this.targetNode instanceof UnaryExpression) {
			UnaryExpression currEx = (UnaryExpression)this.targetNode;
			String opCode = this.getUnaryOperatorCode(currEx);
			Object oper = this.handleChildASTNode(currEx.getChild(1), targetSym);
			toRet = Z3Utils.processUnaryOp(this.targetCtx, oper, opCode);
			isHandled = true;
		}
		if(this.targetNode instanceof UnaryOp) {
			UnaryOp targetOp = (UnaryOp)this.targetNode;
			String opCode = targetOp.getChild(0).getEscapedCodeStr();
			Object oper = this.handleChildASTNode(targetOp.getChild(1), targetSym);
			toRet = Z3Utils.processUnaryOp(this.targetCtx, oper, opCode);
			isHandled = true;
			
		}
		if(this.targetNode instanceof IncDecOp) {
			IncDecOp targetOp = (IncDecOp)this.targetNode;
			ASTNode childNode = null;
			ASTNode operatorNode = null;
			for(ASTNode currChNode: this.targetNode.getChildren()) {
				if(currChNode instanceof IncDec) {
					operatorNode = currChNode;
				} else {
					childNode = currChNode;
				}
			}
			String opCode = operatorNode.getEscapedCodeStr();
			Object oper = this.handleChildASTNode(childNode, targetSym);
			toRet = Z3Utils.processUnaryOp(this.targetCtx, oper, opCode);
			isHandled = true;
		}
		if(!isHandled) {
			System.out.println("FATAL: Unable to handle UnaryExpression:" + this.targetNode.getEscapedCodeStr());
			System.exit(-1);
		}
		return toRet;
	}

}
