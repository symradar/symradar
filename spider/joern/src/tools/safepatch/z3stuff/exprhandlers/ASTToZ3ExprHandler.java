package tools.safepatch.z3stuff.exprhandlers;

import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.Context;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;

/***
 * Common base class that should be specialized to handle 
 * the conversion of different sort of AST expressions.
 *
 * @author machiry
 *
 */
public abstract class ASTToZ3ExprHandler {
	
	protected Logger logger = LogManager.getLogger();
	protected JoernGraphs targetGraphs = null;
	protected ASTNode targetNode = null;
	protected Map<String, Object> requiredValues = null;
	protected Context targetCtx = null;
	protected ASTNode statementNode = null;
	protected ASTNodeToZ3Converter targetConv = null;

	
	protected ASTToZ3ExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statementNode, Map<String, Object> allValues, Context ctx, ASTNodeToZ3Converter mainConv) {
		this.targetGraphs = currGraphs;
		this.targetNode = currNode;
		this.requiredValues = allValues;
		this.targetCtx = ctx;
		this.statementNode = statementNode;
		this.targetConv = mainConv;
		
	}
	
	protected boolean isSymbolInTheNode(ASTNode currNode, String currSym) {
		if(currNode.getEscapedCodeStr().contains(currSym)) {
			if(currNode.getChildren() != null && !currNode.getChildren().isEmpty()) {
				for(ASTNode cNode:currNode.getChildren()) {
					if(isSymbolInTheNode(cNode, currSym)) {
						return true;
					}
				}
				return false;
			}
			return true;
		}
		return false;
	}
	
	/***
	 *  Handle the child AST node of the current node.
	 * @param childNode Child ASTNode to be handled.
	 * @param targetSym target symbol.
	 * @return z3 object of the child AST Node.
	 */
	protected Object handleChildASTNode(ASTNode childNode, String targetSym) {
		ASTToZ3ExprConvertor targetConv = new ASTToZ3ExprConvertor(this.targetGraphs, childNode, this.targetCtx, this.requiredValues, this.statementNode, this.targetConv);
		return targetConv.processASTNode(targetSym);
	}
	
	/***
	 *  Check if the provided symbol have a z3 symbol already generated.
	 * @param val String to be checked.
	 * @return z3 object.
	 */
	protected Object getExistingVal(String val) {
		Object retVal = null;
		if(this.requiredValues.containsKey(val)) {
			retVal = this.requiredValues.get(val);
		}
		return retVal;
	}
	
	/***
	 *  Process the current AST Node.
	 *  This function should be specialized for each type of expression.
	 * @param targetSym target symbol.
	 * @return z3 object.
	 */
	public abstract Object processASTNode(String targetSym);
}
