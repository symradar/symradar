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
 * Handle common comma separated expression.
 * @author machiry
 *
 */
public class CommonExpressionHandler extends ASTToZ3ExprHandler {

	protected CommonExpressionHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statementNode,
			Map<String, Object> allValues, Context ctx, ASTNodeToZ3Converter mainConv) {
		super(currGraphs, currNode, statementNode, allValues, ctx, mainConv);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see tools.safepatch.z3stuff.exprhandlers.ASTToZ3ExprHandler#processASTNode(java.lang.String)
	 */
	@Override
	public Object processASTNode(String targetSym) {
		if(this.targetNode.getChildren() != null) {
			// iterate through each child and find
			// the first child that has it and return it.
			for(ASTNode currChild: this.targetNode.getChildren()) {
				if(this.isSymbolInTheNode(currChild, targetSym)) {
					return this.handleChildASTNode(currChild, targetSym);
				}
			}
		}
		// TODO Auto-generated method stub
		return null;
	}

}
