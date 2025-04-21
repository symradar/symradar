package tools.safepatch.conditions;

import java.io.IOException;

import java.util.HashMap;
import java.util.Map;

import ast.ASTNode;
import ast.expressions.AndExpression;
import ast.expressions.OrExpression;
import ast.expressions.UnaryOp;
import ast.expressions.UnaryOperator;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import tools.safepatch.tests.TestStringFunctionParser;
import tools.safepatch.util.Util;

@Deprecated
public class ConditionToBooleanFormula {

	/*
	 * TODO
	 * - Generalize this to be able to set the converter as a parameter (e.g. to Z3 BoolExpr, to a String boolean expr)
	 */
	public enum CONVERSION_METHOD{
		BY_NODE,
		BY_CODESTRING
	}
	
	private enum BOOLEAN_OPERATOR{
		AND(" & "), OR(" | "), NOT(" ! ");
		
		private String value;
		
		private BOOLEAN_OPERATOR(String value) {
			this.value = value;
		}
		
		public String getValue() {
			return value;
		}
	}
	
	private static final String OPEN_PAR = "(";
	private static final String CLOSED_PAR = ")";
	private static final String C_NOT = "!";
	private static final String SYMBOLS_ALPHABET = "ABCDEFGJKLMNOPQRSTUVWXYZ";
	
	private CONVERSION_METHOD method = CONVERSION_METHOD.BY_NODE;
	private int symbolCount = 0;
	private Map<ASTNode, String> astNodeSymbolMap;
	private Map<String, String> codeStringSymbolMap;
	
	private Condition condition;
	
	public ConditionToBooleanFormula(Condition condition) {
		this.condition = condition;
	}
	
	public ConditionToBooleanFormula(Condition condition, CONVERSION_METHOD m) {
		this.condition = condition;
		this.method = m;
	}
	
	public String convertToBooleanExpression(){
		return convertToBooleanExpression(condition.getChild(0));
	}
	
	private String convertToBooleanExpression(ASTNode n){

		if(n instanceof AndExpression){
			return OPEN_PAR + convertToBooleanExpression(n.getChild(0)) 
			+ BOOLEAN_OPERATOR.AND.getValue() + convertToBooleanExpression(n.getChild(1)) + CLOSED_PAR;
		}
		
		if(n instanceof OrExpression){
			return OPEN_PAR + convertToBooleanExpression(n.getChild(0)) 
			+ BOOLEAN_OPERATOR.OR.getValue() + convertToBooleanExpression(n.getChild(1)) + CLOSED_PAR;
		}
		
		if(n instanceof UnaryOp &&
				n.getChild(0) instanceof UnaryOperator &&
				n.getChild(0).getEscapedCodeStr().equals(C_NOT)){
			return OPEN_PAR + BOOLEAN_OPERATOR.NOT.getValue()  + OPEN_PAR + convertToBooleanExpression(n.getChild(1)) + CLOSED_PAR + CLOSED_PAR; 
		}
		/*
		 * TODO:
		 * - More cases
		 */
		
		return getSymbol(n);
	}
	
	private String getSymbol(ASTNode n) {
		if(this.method == CONVERSION_METHOD.BY_NODE){
			if(this.astNodeSymbolMap == null)
				this.astNodeSymbolMap = new HashMap<ASTNode, String>();
			
			if(astNodeSymbolMap.get(n) == null)
				astNodeSymbolMap.put(n, generateSymbol());
			return astNodeSymbolMap.get(n);
		}
		
		if(this.method == CONVERSION_METHOD.BY_CODESTRING){
			if(this.codeStringSymbolMap == null)
				this.codeStringSymbolMap = new HashMap<String, String>();
			
			if(codeStringSymbolMap.get(n.getEscapedCodeStr()) == null)
				codeStringSymbolMap.put(n.getEscapedCodeStr(), generateSymbol());
			
			return codeStringSymbolMap.get(n.getEscapedCodeStr());
		}
		
		throw new RuntimeException("Boolean symbol mapping method not implemented: " + this.method);
	}

	public String generateSymbol(){
		int length = 1;
		int index = symbolCount;
		if(symbolCount >= SYMBOLS_ALPHABET.length()){ 
			length = symbolCount / SYMBOLS_ALPHABET.length() + 1;
			index = symbolCount % SYMBOLS_ALPHABET.length();
		}
		symbolCount++;
		return Util.repeatString(SYMBOLS_ALPHABET.substring(index, index + 1), length);
	}
	

	
	/*
	 * TEST MAIN
	 */
	public static void main(String[] args) {
		TestStringFunctionParser p = new TestStringFunctionParser("void foo(){\n" + 
				"  if(((asd() > asd->lol.c) || asd->d && !asd->e) || !asd->d){\n" + 
				"    printf(\"Hello!\")\n" + 
				"  }\n" + 
				"}");
		FunctionDef f = p.parse();
		Condition c = (Condition) f.getContent().getChild(0).getChild(0);
		ConditionToBooleanFormula bool = new ConditionToBooleanFormula(c, CONVERSION_METHOD.BY_CODESTRING);
		
		String expr = bool.convertToBooleanExpression();
		System.out.println(expr);
	}
}
