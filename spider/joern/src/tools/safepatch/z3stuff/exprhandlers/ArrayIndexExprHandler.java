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
 * This class handles the conversion of array indexing into z3 obj.
 * i.e., arr[i] -> z3 object
 * @author machiry
 *
 */
public class ArrayIndexExprHandler extends ASTToZ3ExprHandler {

	public ArrayIndexExprHandler(JoernGraphs currGraphs, ASTNode currNode,
			ASTNode statementNode, Map<String, Object> allValues, Context ctx,
			ASTNodeToZ3Converter mainConv) {
		super(currGraphs, currNode, statementNode, allValues, ctx, mainConv);
		// TODO Auto-generated constructor stub
	}

	
	@Override
	public Object processASTNode(String targetSym) {
		try {
			Object leftOp = this.handleChildASTNode(this.targetNode.getChild(0), targetSym);
			Object rightOp = this.handleChildASTNode(this.targetNode.getChild(1), targetSym);
			//  basically, generate z3 object for (arr + i) 
			Object retVal = Z3Utils.processBinOp(this.targetCtx, leftOp, rightOp, "+");
			return retVal;
		} catch(Exception e) {
			logger.debug("Exception occured while trying to generate z3 symbol for {}", this.targetNode.getEscapedCodeStr());
			Object retVal = this.targetConv.generateStringSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
			return retVal;
		}
	}

}
