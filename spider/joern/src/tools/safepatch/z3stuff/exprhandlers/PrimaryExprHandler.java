/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;

import com.microsoft.z3.Context;
import com.microsoft.z3.Z3Exception;

/**
 * This is constant value, convert the const into a z3 obj.
 * @author machiry
 *
 */
public class PrimaryExprHandler extends ASTToZ3ExprHandler {

	public PrimaryExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		Object retVal = null;
		String val = this.targetNode.getEscapedCodeStr();
		if(val.endsWith("L") || val.endsWith("l") || val.endsWith("u") || val.endsWith("U")) {
			val = val.substring(0,  val.length() - 1);
		}
		try {
			if(val.startsWith("0x")) {
				val = val.substring(2);
				long targetVal = Long.parseLong(val, 16);
				retVal = Z3Utils.generateConstant(this.targetCtx, targetVal);
			} else if(val.startsWith("0") && val.length() > 1) {
				long targetVal = Long.parseLong(val.substring(1), 8);
				retVal = Z3Utils.generateConstant(this.targetCtx, targetVal);
			} else {
				retVal = Z3Utils.generateConstant(this.targetCtx, Long.parseLong(val));
			}
		} catch(Exception e) {
			retVal = this.targetConv.generateStringSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
		}
		
		return retVal;
	}

}
