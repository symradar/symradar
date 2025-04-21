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
 * @author machiry
 *
 */
public class IdentifierDeclStExprHandler extends ASTToZ3ExprHandler {

	public IdentifierDeclStExprHandler(JoernGraphs currGraphs,
			ASTNode currNode, ASTNode statementNode,
			Map<String, Object> allValues, Context ctx,
			ASTNodeToZ3Converter mainConv) {
		super(currGraphs, currNode, statementNode, allValues, ctx, mainConv);
		// TODO Auto-generated constructor stub
	}
	
	private boolean hasIdentifierSymbol(ASTNode currChild, String currSymbol) {
		String childst = currChild.getEscapedCodeStr();
		if(childst.equals(currSymbol)) {
			return true;
		}
		ASTNode currC = null;
		for(int i=0; i<currChild.getChildCount(); i++) {
			currC = currChild.getChild(i);
			if(hasIdentifierSymbol(currC, currSymbol)) {
				return true;
			}
		}
		return false;
	}

	/* (non-Javadoc)
	 * @see tools.safepatch.z3stuff.exprhandlers.ASTToZ3ExprHandler#processASTNode(java.lang.String)
	 */
	@Override
	public Object processASTNode(String targetSym) {
		Object retVal = null;
		ASTNode currChild = null;
		Object currVal = null;
		
		for(int i=0; i< this.targetNode.getChildCount(); i++) {
			currChild = this.targetNode.getChild(i);
			currVal = this.handleChildASTNode(currChild, targetSym);
			if(this.hasIdentifierSymbol(currChild, targetSym)) {
				retVal = currVal;
			}
		}
		if(retVal == null) {
			logger.debug("Child name: {} mismatch at {}", targetSym, this.targetNode.getEscapedCodeStr());
			retVal = currVal;
		}
		return retVal;		
	}

}
