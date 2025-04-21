package tools.safepatch.tests;

import java.io.IOException;
import java.util.Stack;

import org.antlr.v4.runtime.ParserRuleContext;

import ast.ASTNode;
import ast.ASTNodeBuilder;
import ast.functionDef.FunctionDef;
import ast.walking.ASTNodeVisitor;
import ast.walking.ASTWalker;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.GumtreeASTMapper;
import tools.safepatch.visualization.GraphvizGenerator;
import tools.safepatch.visualization.GraphvizGenerator.FILE_EXTENSION;

public class TestStringFunctionParser {
	
	private String functionCode;

	private ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	private ModuleParser parser = new ModuleParser(driver);
	private LocalASTWalker astWalker = new LocalASTWalker();

	private GraphvizGenerator vis = new GraphvizGenerator(GraphvizGenerator.DEFAULT_DIR 
			+ "/testFunction", FILE_EXTENSION.PNG);
	
	private boolean visualizationOn = false;

	
	public TestStringFunctionParser(String functionCode) {
		super();
		this.functionCode = functionCode;
		this.parser.addObserver(astWalker);
	}

	
	public GraphvizGenerator getVis() {
		return vis;
	}

	public void activateVisualization(){
		this.visualizationOn = true;
	}

	public void deActivateVisualization(){
		this.visualizationOn = false;
	}
	
	public FunctionDef parse(){
		parser.parseString(functionCode);
		FunctionDef func = this.astWalker.getCurrVisitor().getFunction();
		
		if(visualizationOn)
			try {
				vis.FunctionDefToDot(func);
			} catch (IOException e) {
				e.printStackTrace();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			
		return func;
	}
	
	private class LocalASTNodeVisitor extends ASTNodeVisitor{
		/*
		 * This keeps track of the first function that if finds.
		 */
		private FunctionDef function = null;


		public FunctionDef getFunction() {
			return function;
		}

		@Override
		public void visit(FunctionDef item) {
			
			if (function == null)
				this.function = item;
			
			super.visit(item);
		}

	}
	
	private class LocalASTWalker extends ASTWalker{

		private LocalASTNodeVisitor currVisitor = new LocalASTNodeVisitor();
		
		public LocalASTNodeVisitor getCurrVisitor() {
			return currVisitor;
		}

		@Override
		public void startOfUnit(ParserRuleContext ctx, String filename) {	
		}

		@Override
		public void endOfUnit(ParserRuleContext ctx, String filename) {		
		}

		@Override
		public void processItem(ASTNode node, Stack<ASTNodeBuilder> nodeStack) {
			node.accept(currVisitor);
		}
		
	}
	
	/*
	 * TEST MAIN
	 */
	public static void main(String[] args) {
		String function = "void f(){\n"
				+ "  if (true) {\n"
				+ "    true();\n"
				+ "  } else {\n"
				+ "    false();\n"
				+ "  }\n"
				+ "}"; 
				
		TestStringFunctionParser parser = new TestStringFunctionParser(function);
		parser.activateVisualization();
		FunctionDef f = parser.parse();
		System.out.println(f);
	}
	
}
