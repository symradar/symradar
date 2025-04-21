/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.Context;

import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.expressions.BinaryExpression;
import ast.statements.ExpressionStatement;
import ddg.DataDependenceGraph.DDG;

/**
 * Common class that converts given ASTNode into z3 object.
 * @author machiry
 *
 */
public class ASTToZ3ExprConvertor {
	private static final Logger logger = LogManager.getLogger();
	private JoernGraphs targetGraphs = null;
	private ASTNode targetNode = null;
	private Map<String, Object> requiredValues = null;
	private Context targetCtx = null;
	private ASTNode statementNode = null;
	private ASTNodeToZ3Converter targetConv = null;
	
	/***
	 *  Check if the provided symbol is already computed.
	 * @param symName Symbol name to be checked.
	 * @return true if computed.
	 */
	private boolean isSymbolComputed(String symName) {
		if(this.requiredValues != null) {
			return this.requiredValues.keySet().contains(symName);
		}
		return false;
	}
	
	
	private void addValueToMap(Map<ASTNode, ArrayList<String>> targetMap, ASTNode keyAST, String valueN) {
		ArrayList<String> targetSyms = new ArrayList<String>();
		if(targetMap.containsKey(keyAST)) {
			targetSyms = targetMap.get(keyAST);
		} else {
			targetMap.put(keyAST, targetSyms);
		}
		targetSyms.add(valueN);
	}
	
	/***
	 *  Is the provided ASTNode a binary expression?
	 *  
	 * @param cNode ASTNode to check.
	 * @return true if this is a binary expression else false.
	 */
	private boolean isBinaryExpr(ASTNode cNode) {
		return cNode instanceof BinaryExpression;
	}
	
	public ASTToZ3ExprConvertor(JoernGraphs currGraphs, ASTNode currNode, Context ctx, ASTNodeToZ3Converter parentCaller) {
		this.targetGraphs = currGraphs;
		this.targetNode = currNode;
		this.targetCtx = ctx;
		this.statementNode = currNode;
		this.targetConv = parentCaller;
	}
	
	public ASTToZ3ExprConvertor(JoernGraphs currGraphs, ASTNode currNode, Context ctx, Map<String, Object> allValues, ASTNode parNode, ASTNodeToZ3Converter parentCaller) {
		this(currGraphs, parNode, ctx, parentCaller);
		this.requiredValues = allValues;
		this.targetNode = currNode;
	}
	
	/***
	 *  Get all the required values to convert the current ASTNode to z3 object.
	 *  
	 * @return Map of ASTNode and set of symbols expected from corresponding ASTNode.
	 */
	public Map<ASTNode, ArrayList<String>> getRequiredValues() {
		Map<ASTNode, ArrayList<String>> toRet = new HashMap<ASTNode, ArrayList<String>>();
		DDG funcDDG = this.targetGraphs.getDdg();
		Collection<Object> allSymsUSed = this.targetGraphs.getDefUseCfg().getIdentifierSymbolsUsedBy(this.targetNode);	
		
		for(Object currOb: allSymsUSed) {
			if(currOb instanceof String && !this.isSymbolComputed((String)currOb)) {
				List<ASTNode> defsNodes = funcDDG.getReachingDefs(this.targetNode, (String)currOb);
				if(defsNodes.size() > 0) {
					for(ASTNode currD:defsNodes) {
						this.addValueToMap(toRet, currD, (String)currOb);
					}
				} else {
					// This is a symbol which is used but no node defines it.
					//this.addValueToMap(toRet, this.targetNode, (String)currOb);
					Object symZ3Obj = this.targetConv.generateIdentifierSymbol(this.statementNode, (String)currOb);
					HashMap<String, Object> z3Map = new HashMap<String, Object>();
					z3Map.put((String)currOb, symZ3Obj);
					this.putRequiredValues(this.statementNode, z3Map);
				}
			}
		}
		
		
		return toRet;
	}
	
	/***
	 * Store the required values computed for the given ASTNode.
	 * 
	 * @param currNode ASTNode to process.
	 * @param allSymbols Map of symbol name to z3 object of all the required symbols.
	 */
	public void putRequiredValues(ASTNode currNode, Map<String, Object> allSymbols) {
		for(String symName:allSymbols.keySet()) {
			if(this.requiredValues == null) {
				this.requiredValues = new HashMap<String, Object>();
			}
			if(this.requiredValues.containsKey(symName)) {
				if(allSymbols.get(symName) != (this.requiredValues.get(symName))) {
					// this should never happen.
					// we are getting 2 different z3 objects
					// for the same symbol.
					logger.fatal("Multiple z3 objects received for a single symbol" + symName + " at: " + currNode.getEscapedCodeStr());
					System.exit(-1);
				}
			} else {
				this.requiredValues.put(symName, allSymbols.get(symName));
			}
		}
	}
	
	/***
	 *  Get all the computed intermediate values.
	 * @return Map of string to z3 object for all the intermediate symbols.
	 */
	public Map<String, Object> getAllIntermediateValues() {
		return this.requiredValues;
	}
	
	/***
	 *  Process the current ASTNode into a z3 object and save it
	 *  using the provided symbol name.
	 *  
	 * @param targetSymbol symbol under which the current value needs to be stored.
	 * @return z3 object.
	 */
	public Object processASTNode(String targetSymbol) {
		try {
			ASTToZ3ExprHandler targetHandler = null;
			
			if(this.targetNode instanceof ExpressionStatement) {
				this.targetNode = ((ExpressionStatement)this.targetNode).getExpression();
			}
			
			String typeName = this.targetNode.getTypeAsString();
	
				
				switch (typeName)
				{
					case "AssignmentExpr":
						targetHandler = new AssignmentExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "Condition":
						targetHandler = new CondExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "PtrMemberAccess":
					case "Identifier":
					case "MemberAccess":
						targetHandler = new IdentifierExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "Parameter":
					case "Argument":
					case "ForInit":
						targetHandler = new WrapperExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "IdentifierDeclStatement":
						targetHandler = new IdentifierDeclStExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "IdentifierDecl":
						targetHandler = new IdentiferDeclHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "CallExpression":
						targetHandler = new CallExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "CastExpression":
						targetHandler = new CastExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "UnaryOp":
					case "IncDecOp":
					case "UnaryExpression":
						targetHandler = new UnaryExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "PrimaryExpression":
						targetHandler = new PrimaryExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "ArrayIndexing":
						targetHandler = new ArrayIndexExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "Expression":
						targetHandler = new CommonExpressionHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
					case "ReturnStatement":
						targetHandler = new ReturnStatementHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
						break;
				}
			
				if(targetHandler == null) {
					if(this.isBinaryExpr(this.targetNode)) {
						targetHandler = new BinaryExprHandler(this.targetGraphs, this.targetNode, this.statementNode, this.requiredValues, this.targetCtx, this.targetConv);
					}
				}
			Object retVal = null;
			if(targetHandler == null) {			
				logger.error("Unable to handle ASTNode type: {}, returning TOP", typeName);
				System.out.println("FATAL: Unable to handle AST Node\n");
				System.exit(-1);
			} else {
				retVal = targetHandler.processASTNode(targetSymbol);
			}
			if(retVal == null) {
				retVal = this.targetConv.generateStringSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
			}
			return retVal;
		} catch(Exception e) {
			logger.debug("Exception occured while converting the expression {}", this.targetNode.getEscapedCodeStr());
			e.printStackTrace();
			Object retVal = this.targetConv.generateStringSymbol(this.statementNode, this.targetNode.getEscapedCodeStr());
			return retVal;
		}
	}
}
