/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;

import ast.ASTNode;
import ast.expressions.Argument;
import ast.expressions.ArgumentList;
import ast.expressions.CallExpression;
import ast.expressions.Expression;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;

/**
 * 
 * Handle the conversion of a call expression
 * into z3 expression.
 * 
 * @author machiry
 *
 */
public class CallExprHandler extends ASTToZ3ExprHandler {
	
	public static HashMap<String, Object> functionNameConstant = new HashMap<String, Object>();
	private static final String LIKELY_MACRO = "likely";
	private static final String UNLIKELY_MACRO = "unlikely";
	private static final String WARN_ON_ONCE = "WARN_ON_ONCE";
	
	public CallExprHandler(JoernGraphs currGraphs, ASTNode currNode, ASTNode statNode, Map<String, Object> allValues, 
			Context ctx, ASTNodeToZ3Converter targetConv) {
		super(currGraphs, currNode, statNode, allValues, ctx, targetConv);
	}
	
	/***
	 *  Get an constrained constant unique to each function.
	 *    
	 * @param z3ctx Target z3 context.
	 * @param funcName Function name to which the constant should be generated.
	 * @return Z3 unconstrained constant.
	 */
	public static synchronized Object getFunctionConstant(Context z3ctx, String funcName) {
		if(!functionNameConstant.containsKey(funcName)) {
			functionNameConstant.put(funcName, Z3Utils.getTop(z3ctx));
		}
		return functionNameConstant.get(funcName);
	}
	
	private static boolean isDummyFunc(String funcName) {
		return funcName.equals(LIKELY_MACRO) || funcName.equals(UNLIKELY_MACRO) || funcName.equals(WARN_ON_ONCE);
	}

	@Override
	public Object processASTNode(String targetSym) {
		CallExpression targetCall = (CallExpression)this.targetNode;
		Expression calledFunction = targetCall.getTarget();
		String funcName = calledFunction.getEscapedCodeStr().trim();
		// if this is a known function?
		if(funcName.startsWith("IS_ERR")) {
			ASTNode targetArgNode = targetCall.getChild(1).getChild(0);
			Object calledObj = this.handleChildASTNode(targetArgNode, targetArgNode.getEscapedCodeStr());
			return Z3Utils.processBinOp(this.targetCtx, calledObj, Z3Utils.generateConstant(targetCtx, 0), "==");
			
		}
		// strip off dummy functions.
		if(isDummyFunc(funcName)) {
			ASTNode targetArgNode = targetCall.getChild(1).getChild(0);
			return this.handleChildASTNode(targetArgNode, targetSym);
		}
		
		// else, convert each argument into a z3 object.
		// xor them with a z3 unconstrained variable.
		// basically f(a,b,c) = z3(a) ^ z3(b) ^ z3(c) ^ z3(f)
		
		ArrayList<Object> argZ3Objects = new ArrayList<Object>();
		ArgumentList callArgList = (ArgumentList)targetCall.getChild(1);
		for(int i=0; i<callArgList.getChildCount(); i++) {
			Argument currArg = (Argument)callArgList.getChild(i);
			Object currArgObj = this.handleChildASTNode(currArg, targetSym);
			argZ3Objects.add(currArgObj);
		}
		
		if(argZ3Objects.size() > 0) {
			Object oldZ3Obj = getFunctionConstant(this.targetCtx, funcName);
			for(Object currZ3Obj: argZ3Objects) {
				if(currZ3Obj instanceof BoolExpr) {
					BoolExpr curBExpr = (BoolExpr)currZ3Obj;
					currZ3Obj = Z3Utils.convertBoolToBitVec(targetCtx, curBExpr);
				}
				oldZ3Obj = Z3Utils.processBinOp(this.targetCtx, oldZ3Obj, currZ3Obj, "^");
			}
			return oldZ3Obj;
		} else {
			return this.targetConv.generateStringSymbol(this.targetNode, targetSym);
		}
	}

}
