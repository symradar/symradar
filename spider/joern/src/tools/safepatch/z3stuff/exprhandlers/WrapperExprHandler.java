/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import com.microsoft.z3.Context;

import ast.ASTNode;
import ast.functionDef.Parameter;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;

/**
 * This handles all sorts of wrapper astnodes.
 * @author machiry
 *
 */
public class WrapperExprHandler extends ASTToZ3ExprHandler {

	public WrapperExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		// convert the child astnode into z3 object.
		if(this.targetNode instanceof Parameter) {
			return this.handleChildASTNode(this.targetNode.getChild(1), targetSym);
		} else {
			return this.handleChildASTNode(this.targetNode.getChild(0), targetSym);
		}
	}

}
