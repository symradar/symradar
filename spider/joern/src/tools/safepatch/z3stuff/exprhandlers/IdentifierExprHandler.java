/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import com.microsoft.z3.Context;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.util.Z3Util;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;

/**
 * @author machiry
 *
 */
public class IdentifierExprHandler extends ASTToZ3ExprHandler {

	public IdentifierExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}
	
	/***
	 * If this is a predefined and known constant.
	 * @param ctx Target context.
	 * @param targetStr identifier name.
	 * @return z3 object.
	 */
	public static Object handleKnownPredefinedConstants(Context ctx, String targetStr) {
		if(targetStr.toLowerCase().equals("true")){
			return ctx.mkBV(1, 64);
		}
		if(targetStr.toLowerCase().equals("false")){
			return ctx.mkBV(0, 64);
		}
		if(Z3Util.limits.containsKey(targetStr.trim())) {
			return ctx.mkBV(Z3Util.limits.get(targetStr.trim()), 64);
		}
		return null;
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		
		targetSym = this.targetNode.getEscapedCodeStr();
		// check
		if(this.requiredValues != null && this.requiredValues.containsKey(targetSym))
			return this.requiredValues.get(targetSym);
		
		// Now handle constants.
		Object retVal = handleKnownPredefinedConstants(this.targetCtx, targetSym);
			
		if(retVal != null) {
			this.logger.debug("Handing known constant: {}", targetSym);
			return retVal;
		}
		
		// We need to create new symbol here.
		return this.targetConv.generateIdentifierSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
	}

}
