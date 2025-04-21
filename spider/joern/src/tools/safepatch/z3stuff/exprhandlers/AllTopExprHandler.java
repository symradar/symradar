/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import com.microsoft.z3.Context;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;

/**
 * @author machiry
 *
 */
public class AllTopExprHandler extends ASTToZ3ExprHandler {

	public AllTopExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		String currSymName = this.targetNode.getEscapedCodeStr();
		Object toRet = this.targetConv.generateStringSymbol(this.statementNode, currSymName);
		return toRet;
	}

}
