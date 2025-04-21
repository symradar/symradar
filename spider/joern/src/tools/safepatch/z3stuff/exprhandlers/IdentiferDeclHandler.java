/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.declarations.IdentifierDeclType;
import ast.expressions.AssignmentExpr;
import ast.expressions.Identifier;

import com.microsoft.z3.Context;

/**
 * Handle identifier declaration.
 * like int a => z3object.
 * @author machiry
 *
 */
public class IdentiferDeclHandler extends ASTToZ3ExprHandler {

	public IdentiferDeclHandler(JoernGraphs currGraphs, ASTNode currNode,
			ASTNode statementNode, Map<String, Object> allValues, Context ctx,
			ASTNodeToZ3Converter mainConv) {
		super(currGraphs, currNode, statementNode, allValues, ctx, mainConv);
		
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		ASTNode targetNode = null;
		ASTNode currChild = null;
		// basically get the identifier node and convert the
		// identifier into z3 object.
		for(int i=0; i<this.targetNode.getChildCount(); i++) {
			currChild = this.targetNode.getChild(i);
			if(!(currChild instanceof IdentifierDeclType)) {
				if(currChild instanceof Identifier) {
					targetNode = currChild;
				}
				if(currChild instanceof AssignmentExpr) {
					targetNode = currChild;
				}
			}
		}
		return this.handleChildASTNode(targetNode, targetSym);
	}

}
