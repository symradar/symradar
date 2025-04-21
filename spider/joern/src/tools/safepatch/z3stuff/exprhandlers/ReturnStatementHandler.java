/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import com.microsoft.z3.Context;

import ast.ASTNode;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;

/**
 * @author machiry
 *
 */
public class ReturnStatementHandler extends ASTToZ3ExprHandler {

	protected ReturnStatementHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statementNode,
			Map<String, Object> allValues, Context ctx, ASTNodeToZ3Converter mainConv) {
		super(currGraphs, currNode, statementNode, allValues, ctx, mainConv);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see tools.safepatch.z3stuff.exprhandlers.ASTToZ3ExprHandler#processASTNode(java.lang.String)
	 */
	@Override
	public Object processASTNode(String targetSym) {
		if(this.targetNode.getChildren() != null && !this.targetNode.getChildren().isEmpty()) {
			return this.handleChildASTNode(this.targetNode.getChildren().get(0), targetSym);
		}
		return null;
	}

}
