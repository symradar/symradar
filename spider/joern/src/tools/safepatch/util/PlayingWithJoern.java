package tools.safepatch.util;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Statement;
import ast.walking.ASTNodeVisitor;
import cdg.CDG;
import cdg.CDGCreator;
import cdg.CDGEdge;
import cdg.DominatorTree;
import cfg.ASTToCFGConverter;
import cfg.CFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import ddg.CFGAndUDGToDefUseCFG;
import ddg.DDGCreator;
import ddg.DataDependenceGraph.DDG;
import ddg.DataDependenceGraph.DDGDifference;
import ddg.DataDependenceGraph.DefUseRelation;
import ddg.DefUseCFG.DefUseCFG;
import ddg.DefUseCFG.ReadWriteDbFactory;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.rw.ASTToRWTableConverter;
import tools.safepatch.rw.ReadWriteTable;
import udg.CFGToUDGConverter;
import udg.useDefAnalysis.environments.EmitDefAndUseEnvironment;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDefRecord;

/**
 * @author Eric Camellini
 *
 */
public class PlayingWithJoern {
	
	public static class MyASTNodeVisitor extends ASTNodeVisitor{
		@Override
		public void visit(ASTNode item) {
			System.out.println(item + " " + item.getEscapedCodeStr() + " isInCFG:" + item.isInCFG());
			super.visit(item);
		}
		
	}

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	
	//TO GET CFG OF ONLY ONE STMT
	//	To have the CFG of how all these chunks are linked I could put them in a compound and compute its CFG							
	//	CFG cfg = CCFGFactory.newInstance(stmt);
	//	for (CFGEdge e : cfg.getEdges()){
	//		System.out.println(e);
	//	}
	
	public static void main(String[] args) {
		parser.addObserver(astWalker);
		
		String oldFilename = "../test_files/ddg.c";
		String newFilename = "../test_files/ddg_new.c";
		Path oldPath = Paths.get(oldFilename);
		Path newPath = Paths.get(newFilename);
		String oldPreprocessed = oldPath.toString().replaceAll(oldPath.getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + oldPath.getFileName());
		String newPreprocessed = newPath.toString().replaceAll(newPath.getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + newPath.getFileName());
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(oldFilename, oldPreprocessed);
			parser.parseFile(oldPreprocessed);
			
			prep.preProcess(newFilename, newPreprocessed);
			parser.parseFile(newPreprocessed);
			
			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(oldPreprocessed);
			ArrayList<FunctionDef> newFunctionList = astWalker.getFileFunctionList().get(newPreprocessed);
			
			for (FunctionDef f_old : oldFunctionList)
				for (FunctionDef f_new : newFunctionList)
					
					if(f_new.name.getEscapedCodeStr().equals(f_old.name.getEscapedCodeStr())){
						
						/*
						 * TODO 
						 * Try more tools: tainter, slicing, KNN 
						 */
						System.out.println("________ FUNCTION: " + f_old.name.getEscapedCodeStr() +  " ____________");
						//System.out.println(f);
						//astWalker.currVisitor.defaultHandler(f);

						/*
						 * AST
						 */
						System.out.println("PRINTING OLD AST:");
						new MyASTNodeVisitor().visit((ASTNode) f_old);
						System.out.println();
						System.out.println("PRINTING NEW AST:");
						new MyASTNodeVisitor().visit((ASTNode) f_new);
						
						/*
						 * CFG
						 * Base to build all the other graphs
						 */
						ASTToCFGConverter conv = new ASTToCFGConverter();
						CFG cfg_old = conv.convert(f_old);
						CFG cfg_new = conv.convert(f_new);
						
						System.out.println("\nPRINTING OLD CFG:");
						System.out.println(cfg_old);
						System.out.println("\nPRINTING NEW CFG:");
						System.out.println(cfg_new);


						System.out.println("PRINTING OLD CFG WITH AST NODES:");
						JoernUtil.printCfgWithASTNodes(cfg_old);
						
						System.out.println("\nPRINTING NEW CFG WITH AST NODES:");
						JoernUtil.printCfgWithASTNodes(cfg_new);

						/*
						 * CDG
						 * Useful to understand if a statements depends from a condition in the control flow
						 */
						CDGCreator cdgCreator = new CDGCreator();
						CDG cdg_old = cdgCreator.create(cfg_old);
						CDG cdg_new = cdgCreator.create(cfg_new);
						
						System.out.println("\nPRINTING OLD CDG:");
						JoernUtil.printCdg(cdg_old);
						System.out.println("\nDOMINATOR TREE: ");
						JoernUtil.printDominatorTree(cdg_old.getDominatorTree());
						System.out.println("\nPRINTING NEW CDG:");
						JoernUtil.printCdg(cdg_new);
						System.out.println("\nDOMINATOR TREE: ");
						JoernUtil.printDominatorTree(cdg_new.getDominatorTree());
						
						/*
						 * UDG
						 * Useful to get READ/WRITES by symbol
						 */
						CFGToUDGConverter udgConv = new CFGToUDGConverter();
						UseDefGraph udg_old = udgConv.convert(cfg_old);
						UseDefGraph udg_new = udgConv.convert(cfg_new);

						System.out.println("\nPRINTING OLD UDG FOR EVERY SYMBOL:");
						JoernUtil.printUDG(udg_old);
						System.out.println("\nPRINTING NEW UDG FOR EVERY SYMBOL:");
						JoernUtil.printUDG(udg_new);
						
						/*
						 * DEF-USE CFG
						 * Useful to get READ/WRITES by statement
						 */
						CFGAndUDGToDefUseCFG useDefCfgCreator = new CFGAndUDGToDefUseCFG();
						DefUseCFG defUseCfg_old = useDefCfgCreator.convert(cfg_old, udg_old);
						DefUseCFG defUseCfg_new = useDefCfgCreator.convert(cfg_new, udg_new);
						System.out.println("\nOLD DEF-USE CFG:");
						JoernUtil.printDefUseCFG(defUseCfg_old);
						System.out.println("\nNEW DEF-USE CFG:");
						JoernUtil.printDefUseCFG(defUseCfg_new);
						
						/*
						 * DDG
						 * Not so useful in our case, but can be probably mixed with the CDG to obtain the PDG
						 */
						
						//It doesn't really work ....
						DDGCreator ddgCreator = new DDGCreator();
						DDG ddg_old = ddgCreator.createForDefUseCFG(defUseCfg_old);
						DDG ddg_new = ddgCreator.createForDefUseCFG(defUseCfg_new);
						
						System.out.println("\nPRINTING OLD DDG:");
						JoernUtil.printDdg(ddg_old);
						
						System.out.println("\nPRINTING NEW DDG:");
						JoernUtil.printDdg(ddg_new);
						
						/*
						 * DDG diff is integrated, we don't need it probably...
						 */
						System.out.println("DDG DIFFERENCE:");
						DDGDifference diff = ddg_old.difference(ddg_new);
						JoernUtil.printDdgDiff(diff);

						
						/*
						 * READ WRITE TABLE
//						 */
//						ASTToRWTableConverter rwconverter = new ASTToRWTableConverter();
//						System.out.println();
//						System.out.println("OLD RW:");
//						ReadWriteTable oldRW = rwconverter.convert(f_old);
//						JoernUtil.printReadWriteTable(oldRW);
//						System.out.println();
//						System.out.println("NEW RW:");
//						ReadWriteTable newRW = rwconverter.convert(f_new);
//						JoernUtil.printReadWriteTable(newRW);
						/*
						 * TODO:
						 * If needed these graphs can be mixed (e.g. PDG, can be used in the variable names project
						 */
					}



			Files.delete(Paths.get(oldPreprocessed));
			Files.delete(Paths.get(newPreprocessed));
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	//	private static void visitAST(ASTNode node, ASTNodeVisitor visitor){
	//		visitor.defaultHandler(node);
	//		for (int i = 0; i < node.getChildCount(); i++)
	//			visitAST(node.getChild(i), visitor);
	//	}

}

