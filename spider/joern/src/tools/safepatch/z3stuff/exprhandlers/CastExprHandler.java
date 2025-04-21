/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;

import com.microsoft.z3.Context;

/**
 * 
 * Convert the cast expression into z3 object.
 * Example: (int*)&a => z3convert(&a)
 * @author machiry
 *
 */
public class CastExprHandler extends ASTToZ3ExprHandler {

	public CastExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		// just convert the source operand into z3 object.
		return this.handleChildASTNode(this.targetNode.getChild(1), targetSym);
	}

}
