package tools.safepatch.util;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.declarations.IdentifierDecl;
import ast.functionDef.Parameter;
import ast.expressions.ArrayIndexing;
import ast.expressions.AssignmentExpr;
import ast.expressions.CastExpression;
import ast.expressions.Expression;
import ast.expressions.PtrMemberAccess;
import ast.functionDef.FunctionDef;
import ast.functionDef.ParameterList;
import ast.statements.IdentifierDeclStatement;
import ast.walking.ASTNodeVisitor;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;


public class TypeInference {

	private static final Logger logger = LogManager.getLogger();
	
	private static class TypeInfVisitor extends ASTNodeVisitor{
		protected Map<String, String> declaredVariables = new HashMap<String, String>();
		protected List<AssignmentExpr> assignments = new LinkedList<AssignmentExpr>();
		 
		@Override
		public void visit(ParameterList param){
			super.visit(param);
			if(param.getChildCount() > 0)
				for(ASTNode child : param.getChildren()){
					String type = ((Parameter)child).type.getType();
					String name = ((Parameter)child).name.getEscapedCodeStr();
					if(logger.isDebugEnabled()) logger.debug("Visiting parameter: {} {}", name, type);
					this.declaredVariables.put(name, type);
				}
		}
		
		@Override
		public void visit(IdentifierDeclStatement decl){
			super.visit(decl);
			for(ASTNode child : decl.getChildren()){
				String type = ((IdentifierDecl)child).getType().getEscapedCodeStr();
				String name = ((IdentifierDecl)child).getName().getEscapedCodeStr();
				if(logger.isDebugEnabled()) logger.debug("Visiting declaration: {} {}", name, type);
				this.declaredVariables.put(name, type);
			}
		}
		
		@Override
		public void visit(AssignmentExpr expression) {
			super.visit(expression);
			if(logger.isDebugEnabled()) logger.debug("Visiting assignment expr: {}", expression);
			this.assignments.add(expression);
		}
	}
		
	static String inferLiteral(Expression e) {
		String type = null;
		// var = n, n.n1, "n"
		if (e instanceof ast.expressions.PrimaryExpression){
			// to be sound we store int rather then unsigned int.
			type = "int";
			if (e.getEscapedCodeStr().contains(".")){
				type = "float";
			}
			if (e.getEscapedCodeStr().contains("\"")){
				type = "string";
			}
		}
		
		// var = -var, -n
		//TODO sure?
		else if (e instanceof ast.expressions.UnaryOp && 
				e.getChildren().get(0).getEscapedCodeStr().equals("-")){
			type = "int";
		}
		
		if(logger.isDebugEnabled()) logger.debug("Inferred literal: {}", type);
		return type;
	}
	
	// look in the rhs of an expression to see if there is a variable or a literal whose type
	// is known
	static String inferTypeRightSide(Expression expr, Map<String, String> declaredVariables){
		if(logger.isDebugEnabled()) logger.debug("inferring right: {}", expr);
		if(expr instanceof CastExpression){
			if(logger.isDebugEnabled()) logger.debug("Cast: ", ((CastExpression) expr).getCastTarget().getEscapedCodeStr());
			return ((CastExpression) expr).getCastTarget().getEscapedCodeStr();
		}
		
		for (ASTNode n : expr.getNodes()){
//			Expression e = (Expression) n;
//			String rhs = GetLiteral(e);
//			
//			if (rhs != null){
//				// literal case
//				return rhs;
//			} else {
//				//var = var
//				if (e instanceof ast.expressions.Identifier) {
//					rhs = ((ast.expressions.Identifier)e).getLabel();
//					if (decl_var.containsKey(rhs)) {
//						return decl_var.get(rhs);
//					}
//				}
//			}
			if(declaredVariables.containsKey(n.getEscapedCodeStr())){
				if(logger.isDebugEnabled()) logger.debug("Inferred {} from {}", declaredVariables.get(n.getEscapedCodeStr()), n.getEscapedCodeStr());
				return declaredVariables.get(n.getEscapedCodeStr());
			}
		}
		
		//TODO: check type cast
		return inferLiteral(expr);
	}

//	static boolean setRight(Expression expr, String startType, Map<String, String> declaredVariables){
//		if(logger.isDebugEnabled()) logger.debug("setting right: {}", expr);
//		boolean inserted = false;
//		
//		for (ASTNode n : expr.getLeaves()){
//			Expression e = (Expression) n;
//			if (e instanceof ast.expressions.Identifier) {
//				ASTNode parent = e.getParent();
//				if (parent instanceof ast.expressions.UnaryOp &&
//						parent.getChildren().get(0).getEscapedCodeStr().equals("*")){
//					if (!startType.contains("*"))
//						startType = startType + " *";
//				}
//				
//				String rhs = "";
//				// member access
//				if (parent instanceof ast.expressions.MemberAccess) {
//					rhs = parent.getEscapedCodeStr();
//				}
//				else {
//					rhs = ((ast.expressions.Identifier)e).getLabel();
//				}
//				if (!declaredVariables.containsKey(rhs)) {
//					 declaredVariables.put(rhs, startType);
//					 inserted = true;
//				}
//			}	
//		}
//		return inserted;
//	}
	
	static String getLabel(Expression e) {
		if (e instanceof ast.expressions.MemberAccess || 
				e instanceof PtrMemberAccess ||
				e instanceof ArrayIndexing ||
				(e instanceof ast.expressions.UnaryOp && 
				(e.getChildren().get(0).getEscapedCodeStr().equals("*") || 
						e.getChildren().get(0).getEscapedCodeStr().equals("&"))))
			return e.getEscapedCodeStr();		
		else if (e instanceof ast.expressions.Identifier)
			return  e.getLabel();
			
		return null;
	}
	
	public static Map<String, String> getTypeInference(FunctionDef f) {
		
		TypeInfVisitor tv = new TypeInfVisitor();
		tv.visit(f);
		
		// get declared variables and assignment expressions
		String left = "";
		//String right = "";
		boolean fixpoint = false;
		
		while (!fixpoint) {
			if(logger.isDebugEnabled()) logger.debug("Fixpoint not reached, re-iterating.");
			fixpoint = true;
			
			//For every assignment
			for (AssignmentExpr expr : tv.assignments){
				if(logger.isDebugEnabled()) logger.debug("Handling {}", expr);
				
				Expression eLeft = expr.getLeft();
				Expression eRight = expr.getRight();
		
				// set the left hand side
				left = getLabel(eLeft);
				if(logger.isDebugEnabled()) logger.debug("left label: {}", left);
				if (left == null)
					continue;
					
				// Infer the left from the right
				if (!tv.declaredVariables.containsKey(left)){
					if(logger.isDebugEnabled()) logger.debug("Inferring left from right");
					String type = inferTypeRightSide(eRight, tv.declaredVariables);
					if (type != null){
						tv.declaredVariables.put(left, type);
					    fixpoint = false;
					}
				} else if(logger.isDebugEnabled()) logger.debug("Type already known.");
				
				// Infer the right from the left
//				if (tv.declaredVariables.containsKey(left) && !tv.declaredVariables.containsKey(right)){
//					if(logger.isDebugEnabled()) logger.debug("Inferring right from left");
//					//tv.declaredVariables.put(rhs, tv.declaredVariables.get(lhs));
//					if (setRight(eRight, tv.declaredVariables.get(left), tv.declaredVariables))
//						fixpoint = false;
//				}

			}
		
		}
		
		if(logger.isDebugEnabled()) logger.debug("DONE!");
		for (Map.Entry<String, String> entry : tv.declaredVariables.entrySet()) {
		    if(logger.isDebugEnabled()) logger.debug(entry.getKey()+" : "+entry.getValue());
		}
		
		return tv.declaredVariables;
	}
	
	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	
	/* Z3 renamer tester */
	public static void main(String[] args) {
		parser.addObserver(astWalker);
		
		String func_name = "main";
		String filename = "../test_files/type_inf.c"; // FIXME: take input
		Path Path = Paths.get(filename);
		String Preprocessed = Path.toString().replaceAll(Path.getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + Path.getFileName());
		
		// END FIXME
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(filename, Preprocessed);
			parser.parseFile(Preprocessed);
			
			
			ArrayList<FunctionDef> FunctionList = astWalker.getFileFunctionList().get(Preprocessed);
			
			for (FunctionDef f : FunctionList)
			{	
				if(!f.name.getEscapedCodeStr().equals(func_name)){
					continue;
				}
				
				getTypeInference(f);

			}
	
				Files.delete(Paths.get(Preprocessed));
			} catch (Exception e) {
				e.printStackTrace();
			}


	}

}

