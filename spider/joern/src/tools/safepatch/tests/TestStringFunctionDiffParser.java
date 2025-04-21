package tools.safepatch.tests;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Stack;

import org.antlr.v4.runtime.ParserRuleContext;

import ast.ASTNode;
import ast.ASTNodeBuilder;
import ast.functionDef.FunctionDef;
import ast.walking.ASTNodeVisitor;
import ast.walking.ASTWalker;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTNodeVisitor;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.GumtreeASTMapper;
import tools.safepatch.visualization.GraphvizGenerator;
import tools.safepatch.visualization.GraphvizGenerator.FILE_EXTENSION;

/**
 * @author Eric Camellini
 *
 */
public class TestStringFunctionDiffParser {

	private String oldFunctionCode;
	private String newFunctionCode;

	private ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	private ModuleParser parser = new ModuleParser(driver);
	private LocalASTWalker astWalker = new LocalASTWalker();

	private GraphvizGenerator vis = new GraphvizGenerator(GraphvizGenerator.DEFAULT_DIR 
			+ "/testFunctionDiff", FILE_EXTENSION.PNG);
	
	private boolean visualizationOn = false;
	
	public TestStringFunctionDiffParser(String oldFunctionCode, String newFunctionCode) {
		this.oldFunctionCode = oldFunctionCode;
		this.newFunctionCode = newFunctionCode;
		this.parser.addObserver(astWalker);
	}

	public void activateVisualization(){
		this.visualizationOn = true;
	}

	public void deActivateVisualization(){
		this.visualizationOn = false;
	}
	
	public FunctionDiff parse(){
		parser.parseString(oldFunctionCode);
		parser.parseString(newFunctionCode);
		FunctionDef oldFunc = this.astWalker.getCurrVisitor().getOldFunction();
		FunctionDef newFunc = this.astWalker.getCurrVisitor().getNewFunction();
		

		FunctionDiff f = new FunctionDiff(null, oldFunc, null, newFunc);
		
		f.generateFineGrainedDiff();
		f.generateGraphs();
		f.generateReadWriteInformation();
		
		assert(f.getFineGrainedDiff().getMappedNode(oldFunc).equals(newFunc));
		
		//Visualization
		if(visualizationOn)
			try {
				this.vis.functionDiffToDot(f);
			} catch (IOException | InterruptedException e) {
				e.printStackTrace();
			}
		
		return f;

	}
	
	private class LocalASTNodeVisitor extends ASTNodeVisitor{

		private FunctionDef oldFunction = null;
		private FunctionDef newFunction = null;

		public FunctionDef getOldFunction() {
			return oldFunction;
		}


		public FunctionDef getNewFunction() {
			return newFunction;
		}

		@Override
		public void visit(FunctionDef item) {
			if (oldFunction == null)
				this.oldFunction = item;
			else if (newFunction == null)
				this.newFunction = item;
			
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
		String oldFunction = "void f(){\n"
				+ "  if (true) {\n"
				+ "    true();\n"
				+ "  } else {\n"
				+ "    false();\n"
				+ "  }\n"
				+ "}"; 
		
		String newFunction = "void f(){\n"
				+ "  if (false) {\n"
				+ "    true();\n"
				+ "  } else {\n"
				+ "    false();\n"
				+ "  }\n"
				+ "}"; 
		
		TestStringFunctionDiffParser parser = new TestStringFunctionDiffParser(oldFunction, newFunction);
		parser.activateVisualization();
		FunctionDiff diff = parser.parse();
	}
	
}
