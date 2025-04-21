package tools.safepatch.conditions;

import java.util.HashMap;
import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.ArithExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.IntExpr;
import com.microsoft.z3.Z3Exception;

import ast.ASTNode;
import ast.expressions.AdditiveExpression;
import ast.expressions.AndExpression;
import ast.expressions.AssignmentExpr;
import ast.expressions.BinaryExpression;
import ast.expressions.CallExpression;
import ast.expressions.CastExpression;
import ast.expressions.Identifier;
import ast.expressions.OrExpression;
import ast.expressions.PrimaryExpression;
import ast.expressions.UnaryOp;
import ast.expressions.UnaryOperator;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import ast.statements.ExpressionStatement;
import ast.statements.IdentifierDeclStatement;
import tools.safepatch.tests.TestStringFunctionParser;
import tools.safepatch.util.Util;
import tools.safepatch.util.Z3Util;


public class ConditionToZ3Converter {

	/*
	 * TODO
	 * - Generalize this to be able to set the converter as a parameter (e.g. to Z3 BoolExpr, to a String boolean expr)
	 */
	private static final Logger logger = LogManager.getLogger();

	Context ctx;

	private static final String C_NOT = "!";
	private static final String SYMBOLS_ALPHABET = "ABCDEFGJKLMNOPQRSTUVWXYZ";
	private static final String LIKELY_MACRO = "likely";
	private static final String UNLIKELY_MACRO = "unlikely";

	private Map<String, Expr> codestringExprMap;
	private int symbolCount = 0;

	private boolean boolOnly = true;
	private boolean useReal = false;

	private int BV_SIZE = 32;
	private boolean bvEnabled = true;
	
	/*
	 * This map contains constraints for
	 * symbols found in arithmetic 
	 * expressions. 
	 * (code strings, keys in the map).
	 */
	public Map<String, Object> constraintsMap = null;
	
	public ConditionToZ3Converter(Context ctx) {
		this.ctx = ctx;
	}	

	public void setConstraintsMap(Map<String, Object> constraintsMap){
		this.constraintsMap = constraintsMap;
	}
	
	public void setBVEnabled(boolean value){
		this.bvEnabled = value;
	}
	
	public void setBoolOnly(boolean b){
		this.boolOnly = b;
	}

	public boolean getBoolOnly(){
		return boolOnly;
	}

	public boolean isUseReal() {
		return useReal;
	}

	public void setUseReal(boolean useReal) {
		this.useReal = useReal;
	}

	public Map<String, Expr> getCodestringExprMap() {
		return codestringExprMap;
	}

	/*
	 * Converts the condition in a propositional logic
	 * expression that contains only OR, AND, NOT operators.
	 * All the rest of the stuff is converted into Z3 Bool constants.
	 * Two elements of the condition that have the same code string are converted
	 * to the same constant.
	 */
	public BoolExpr getZ3BoolExpr(Condition condition) {
		ASTNode conditionContent;
		if(condition.getChild(0) instanceof CallExpression &&
				(condition.getChild(0).getChild(0).getEscapedCodeStr().equals(LIKELY_MACRO) ||
						condition.getChild(0).getChild(0).getEscapedCodeStr().equals(UNLIKELY_MACRO))){
			conditionContent = condition.getChild(0).getChild(1).getChild(0).getChild(0);
		} else {
			conditionContent = condition.getChild(0);
		}
		BoolExpr retval = getZ3BoolExprHelper(conditionContent);
		if(logger.isDebugEnabled()) logger.debug("Conversion done. Returning: {}", retval);
		return retval;
	}

	private BoolExpr getZ3BoolExprHelper(ASTNode n) {
		if(logger.isDebugEnabled()) logger.debug("Node: {}", n);
		
		if(n instanceof CastExpression){
			//TODO handle some casts (e.g. to real or int)
			if(logger.isDebugEnabled()) logger.debug("Ignoring cast.");
			return getZ3BoolExprHelper(n.getChild(1));
		}
		
		if(n instanceof AndExpression){
			if(logger.isDebugEnabled()) logger.debug("Making AND.");
			return ctx.mkAnd(getZ3BoolExprHelper(n.getChild(0)), getZ3BoolExprHelper(n.getChild(1)));
		}

		if(n instanceof OrExpression){
			if(logger.isDebugEnabled()) logger.debug("Making OR.");
			return ctx.mkOr(getZ3BoolExprHelper(n.getChild(0)), getZ3BoolExprHelper(n.getChild(1)));
		}

		if(n instanceof UnaryOp &&
				n.getChild(0) instanceof UnaryOperator &&
				n.getChild(0).getEscapedCodeStr().equals(C_NOT)){
			if(logger.isDebugEnabled()) logger.debug("Making NOT.");
			return ctx.mkNot(getZ3BoolExprHelper(n.getChild(1)));
		}

		if(boolOnly){
			return getBoolConst(n);
		} else {
			if(n instanceof BinaryExpression){
				String opcode = n.getOperatorCode();
				if(logger.isDebugEnabled()) logger.debug("BinaryExpression opcode: {}", opcode);
				switch (opcode) {
				case ">":
					if(logger.isDebugEnabled()) logger.debug("Making >");
					return ctx.mkGt(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

				case "<":
					if(logger.isDebugEnabled()) logger.debug("Making <");
					return ctx.mkLt(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

				case ">=":
					if(logger.isDebugEnabled()) logger.debug("Making >=");
					return ctx.mkOr(
							ctx.mkGt(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1))),
							ctx.mkEq(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1))));

				case "<=":
					if(logger.isDebugEnabled()) logger.debug("Making <=");
					return ctx.mkOr(
							ctx.mkLt(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1))),
							ctx.mkEq(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1))));

				case "==":
					if(logger.isDebugEnabled()) logger.debug("Making ==");
					return ctx.mkEq(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

				case "!=":
					if(logger.isDebugEnabled()) logger.debug("Making !=");
					return ctx.mkNot(ctx.mkEq(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1))));

				default:
					if(logger.isDebugEnabled()) logger.debug("Unknown boolean BinaryOperator, converting to arith expr != 0");
					return ctx.mkNot(ctx.mkEq(getZ3ArithExprHelper(n), ctx.mkInt(0)));
				}
			}
		}

		//return getBoolConst(n);
		return ctx.mkNot(ctx.mkEq(getZ3ArithExprHelper(n), ctx.mkInt(0)));
	}

	private ArithExpr getZ3ArithExprHelper(ASTNode n) {
		if(logger.isDebugEnabled()) logger.debug("Node: {}", n);
		
		if(n instanceof CastExpression){
			//TODO handle some casts (e.g. to real or int)
			if(logger.isDebugEnabled()) logger.debug("Ignoring cast.");
			return getZ3ArithExprHelper(n.getChild(1));
		}
		
		if(n instanceof BinaryExpression){

			//TODO check if casting to IntExpr here can result in some problems

			String opcode = n.getOperatorCode();
			if(logger.isDebugEnabled()) logger.debug("BinaryExpression opcode: ", opcode);
			switch (opcode) {
			case "+":
				if(logger.isDebugEnabled()) logger.debug("Making +");
				return ctx.mkAdd(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

			case "-":
				if(logger.isDebugEnabled()) logger.debug("Making -");
				return ctx.mkSub(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));


			case "*":
				if(logger.isDebugEnabled()) logger.debug("Making *");
				return ctx.mkMul(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

			case "/":
				if(logger.isDebugEnabled()) logger.debug("Making /");
				return ctx.mkDiv(getZ3ArithExprHelper(n.getChild(0)), getZ3ArithExprHelper(n.getChild(1)));

			case "%":
				if(logger.isDebugEnabled()) logger.debug("Making %");
				return ctx.mkMod((IntExpr) getZ3ArithExprHelper(n.getChild(0)), (IntExpr) getZ3ArithExprHelper(n.getChild(1)));

			case "&":
				if(logger.isDebugEnabled()) logger.debug("Making &");
				if(this.bvEnabled){
					return ctx.mkBV2Int(ctx.mkBVAND(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0))), 
							ctx.mkInt2BV(this.BV_SIZE , 
									(IntExpr) getZ3ArithExprHelper(n.getChild(1)))), true);
				} else return generateArithExprDefault(n);

			case "|":
				if(this.bvEnabled){
					if(logger.isDebugEnabled()) logger.debug("Making |");
					return ctx.mkBV2Int(ctx.mkBVOR(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0))), 
							ctx.mkInt2BV(this.BV_SIZE , 
									(IntExpr) getZ3ArithExprHelper(n.getChild(1)))), true);
				} else return generateArithExprDefault(n);

			case "^":
				if(this.bvEnabled){
					if(logger.isDebugEnabled()) logger.debug("Making ^");
					return ctx.mkBV2Int(ctx.mkBVXOR(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0))), 
							ctx.mkInt2BV(this.BV_SIZE , 
									(IntExpr) getZ3ArithExprHelper(n.getChild(1)))), true);
				} else return generateArithExprDefault(n);

			case "<<":
				if(this.bvEnabled){
					if(logger.isDebugEnabled()) logger.debug("Making <<");
					return ctx.mkBV2Int(ctx.mkBVSHL(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0))), 
							ctx.mkInt2BV(this.BV_SIZE , 
									(IntExpr) getZ3ArithExprHelper(n.getChild(1)))), true);
				} else return generateArithExprDefault(n);

			case ">>":
				if(this.bvEnabled){
					if(logger.isDebugEnabled()) logger.debug("Making >> (arithmetic)");
					return ctx.mkBV2Int(ctx.mkBVASHR(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0))), 
							ctx.mkInt2BV(this.BV_SIZE , 
									(IntExpr) getZ3ArithExprHelper(n.getChild(1)))), true);
				} else return generateArithExprDefault(n);

			default:
				if(logger.isDebugEnabled()) logger.debug("Unknown BinaryOperator");
			}
		}

		if(n instanceof PrimaryExpression){
			if(logger.isDebugEnabled()) logger.debug("Making numeric const for {}", n);
			String val = n.getEscapedCodeStr();
			
			// TODO solve sign problems? 
			// Removing the possible L at the end
			if(val.endsWith("L") || val.endsWith("l")
					|| val.endsWith("u") || val.endsWith("U")) val = val.substring(0, val.length() - 1);
			
			try {
				if(logger.isDebugEnabled()) logger.debug("Trying int");
				if(val.startsWith("0x")) return ctx.mkInt(Long.parseLong(val.substring(2), 16));
				else if(val.startsWith("0") && 
						!val.equals("0")) return ctx.mkInt(Long.parseLong(val.substring(1), 8));
				else return ctx.mkInt(val);
			} catch (Z3Exception e) {
				try {
					if(logger.isDebugEnabled()) logger.debug("Failed with int, trying float");
					return ctx.mkReal(val);
				} catch (Z3Exception e1) {
					if(logger.isDebugEnabled()) logger.debug("Failed with float, handling as default arith expr");
					return generateArithExprDefault(n);
				}
			}
		}

		if (n instanceof UnaryOp &&
				n.getChild(0) instanceof UnaryOperator){
			String opcode = n.getChild(0).getEscapedCodeStr();
			if(logger.isDebugEnabled()) logger.debug("UnaryOperator opcode: {}", opcode);
			switch (opcode) {
			case "-":
				if(logger.isDebugEnabled()) logger.debug("Making Unary -");
				return ctx.mkUnaryMinus(getZ3ArithExprHelper(n.getChild(1)));

			case "+":
				if(logger.isDebugEnabled()) logger.debug("Making Unary +");
				return ctx.mkAdd(getZ3ArithExprHelper(n.getChild(1)));

			case "--":
				//TODO
				return generateArithExprDefault(n);

			case "++":
				//TODO
				return generateArithExprDefault(n);

			case "~":
				if(logger.isDebugEnabled()) logger.debug("Making ~");
				if(this.bvEnabled){
					return ctx.mkBV2Int(ctx.mkBVNeg(
							ctx.mkInt2BV(this.BV_SIZE, 
									(IntExpr) getZ3ArithExprHelper(n.getChild(0)))), true);
				} else return generateArithExprDefault(n);

			default:
				if(logger.isDebugEnabled()) logger.debug("UnaryOperator not handled by the conversion");
				break;
			}
		}

		if(n instanceof Identifier){
			if(Z3Util.limits.keySet().stream().anyMatch(l -> l.equals(n.getEscapedCodeStr()))){
				if(logger.isDebugEnabled()) logger.debug("Making const: {} {}", n.getEscapedCodeStr(), Z3Util.limits.get(n.getEscapedCodeStr()));
				return ctx.mkInt(Z3Util.limits.get(n.getEscapedCodeStr()));
			}
			
			if(n.getEscapedCodeStr().toLowerCase().equals("true")){
				if(logger.isDebugEnabled()) logger.debug("Making const: {} {}", n.getEscapedCodeStr(), 1);
				return ctx.mkInt(1);
			}
			if(n.getEscapedCodeStr().toLowerCase().equals("false")){
				if(logger.isDebugEnabled()) logger.debug("Making const: {} {}", n.getEscapedCodeStr(), 0);
				return ctx.mkInt(0);
			}
		}

		return generateArithExprDefault(n);
	}

	private ArithExpr generateArithExprDefault(ASTNode n) {
		
		/* TODO handle FunctionCalls and ++ -- or stuff like that,
		* for which two same code-strings shouldn't result in the 
		* same value. This could be done through another table.
		* The problem is that maybe we want to generate the same
		* Expr in some cases (e.g. when a function is called
		* two times in the same line).
		*/
		
		//Handling constraintsTable
		if(this.constraintsMap != null &&
				this.constraintsMap.get(n.getEscapedCodeStr()) != null){
			ArithExpr constraintResult = handleConstraint(n);
			if(constraintResult != null) return constraintResult;
		}
		
		/* If there are no constraints in the table we
		 just generate a new unconstrained ArithExpr linked to the
		 code-string of the node, or take it from the related
		 table if it's already there.
		 */
		 
		if(this.codestringExprMap == null)
			this.codestringExprMap = new HashMap<String, Expr>();
		if(this.codestringExprMap.get(n.getEscapedCodeStr()) == null){
			if(this.useReal) this.codestringExprMap.put(n.getEscapedCodeStr(), ctx.mkRealConst(
					generateSymbol()));
			else this.codestringExprMap.put(n.getEscapedCodeStr(), ctx.mkIntConst(
					generateSymbol()));
		}

		Expr retval = this.codestringExprMap.get(n.getEscapedCodeStr());
		if(logger.isDebugEnabled()) logger.debug("Returning {}", retval);
		return (ArithExpr) retval;
	}

	private ArithExpr handleConstraint(ASTNode n) {
		if(logger.isDebugEnabled()) logger.debug("Handling constraint for {}", n.getEscapedCodeStr());
		Object constraint = this.constraintsMap.get(n.getEscapedCodeStr());
		
		if(logger.isDebugEnabled()) logger.debug("Constraint found: {}", constraint);
		
		if(constraint == null) return null;
		
		if(constraint instanceof ASTNode){
			/* In this case it means that an ASTNode was put directly in the
			* constraints, so it was something directly taken from the AST
			* and ready to be converted in a Z3 arithmetic expression.
			* These constraints are the results of assignments.
			* (e.g. we know that a = b + 1 and a is used in the condition,
			* there will be a constraint for a that points to b + 1 in the AST
			* and can be directly handled.
			* 
			* All the other types of constraints are not yet implemented
			* and they will be by filling the constraints table with other kind
			* of Objects or handled outside this class.
			*/
			return this.getZ3ArithExprHelper((ASTNode) constraint);
		} else return null;
	}

	private BoolExpr getBoolConst(ASTNode n) {
		/* 
		 * This method doesn't check for constraints, since
		 * constraints are there only for Arithmetic variables.
		*/
		
		/*
		 * Generate a new unconstrained BoolExpr linked to the
		 * code-string of the node, or take it from the related
		 * table if it's already there.
		 */
		if(this.codestringExprMap == null)
			this.codestringExprMap = new HashMap<String, Expr>();
		if(this.codestringExprMap.get(n.getEscapedCodeStr()) == null)
			this.codestringExprMap.put(n.getEscapedCodeStr(), ctx.mkBoolConst(
					generateSymbol()));

		Expr retval = this.codestringExprMap.get(n.getEscapedCodeStr());
		if(logger.isDebugEnabled()) logger.debug("Returning {}", retval);
		return (BoolExpr) retval;
	}

	private String generateSymbol(){
		int length = 1;
		int index = symbolCount;
		if(symbolCount  >= SYMBOLS_ALPHABET.length()){ 
			length = symbolCount / SYMBOLS_ALPHABET.length() + 1;
			index = symbolCount % SYMBOLS_ALPHABET.length();
		}
		symbolCount++;

		String retval = Util.repeatString(SYMBOLS_ALPHABET.substring(index, index + 1), length);
		if(logger.isDebugEnabled()) logger.debug("Generating Z3 symbol: {}", retval);
		return retval;
	}


	/*
	 * TEST MAIN
	 */
	public static void main(String[] args) {
		
		String func = "void foo(){\n" + 
				"  %s\n" +
				"  if(%s){\n" + 
				"    printf(\"Hello!\")\n" + 
				"  }\n" + 
				"}";
		String constraints = "b = 11;\n  a = b + 1;";
		String condition = "a > 10";
		
		func = String.format(func, constraints, condition);
		System.out.println(func);
		TestStringFunctionParser p = new TestStringFunctionParser(func);
		FunctionDef f = p.parse();
		
		
		System.out.println();
		int count = constraints.split("\n").length;
		HashMap<String, Object> constraintsMap = new HashMap<String, Object>();
		System.out.println(count + " constraints to add:");
		for(int i = 0 ; i < count ; i++){
			ASTNode stmt =  f.getContent().getChild(i);
			System.out.println("Statement: " + stmt);
			if(stmt instanceof ExpressionStatement && stmt.getChild(0) instanceof AssignmentExpr){
				AssignmentExpr assigment = (AssignmentExpr) stmt.getChild(0);
				constraintsMap.put(assigment.getLeft().getEscapedCodeStr(), assigment.getRight());
			}
		}
		
		System.out.println();
		System.out.println("Constraints added: ");
		System.out.println(constraintsMap);
		
		System.out.println();
		Condition c = (Condition) f.getContent().getChild(count).getChild(0);
		System.out.println("Condition: ");
		System.out.println(c);
		
		
		ConditionToZ3Converter conv = new ConditionToZ3Converter(new Context());

		conv.setConstraintsMap(constraintsMap);
		conv.setBoolOnly(false);
		//let's try to add some constraints:

		System.out.println();
		
		BoolExpr expr = conv.getZ3BoolExpr(c);
		System.out.println("Generated Z3 symbols:");
		System.out.println(conv.getCodestringExprMap());
		
		System.out.println();
		System.out.println("Converted condition:");
		System.out.println(expr);
	}
}
