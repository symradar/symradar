package tools.safepatch.util;
import neo4j.traversals.readWriteDB.Traversals;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

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
import cfg.C.CCFG;
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
import tools.safepatch.cfgimpl.CFGIterator;
import tools.safepatch.cfgimpl.ConditionsConstraintsExtractor;
import tools.safepatch.rw.ASTToRWTableConverter;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.visualization.GraphvizGenerator;
import udg.CFGToUDGConverter;
import udg.useDefAnalysis.environments.EmitDefAndUseEnvironment;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDefRecord;
import org.neo4j.graphdb.index.IndexHits;

import com.github.gumtreediff.gen.php.PhpParser.negateOrCast_return;

import org.neo4j.graphdb.Node;

public class Example {

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

		//		String filename = "../examples/android_security_bulletin/08-2016/CVE-2014-9880/old_latest/venc.c";
		String filename = "../test_files/test.c";
		//String filename = "../test_files/cfg.c";
		Path Path = Paths.get(filename);
		String Preprocessed = Path.toString().replaceAll(Path.getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + Path.getFileName());
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(filename, Preprocessed);
			parser.parseFile(Preprocessed);


			ArrayList<FunctionDef> FunctionList = astWalker.getFileFunctionList().get(Preprocessed);

			GraphvizGenerator gv = new GraphvizGenerator(GraphvizGenerator.DEFAULT_DIR + "/example");

			for (FunctionDef f : FunctionList)
			{	

				System.out.println("FUNCTION: " + f.getEscapedCodeStr());
				System.out.println();
				gv.FunctionDefToDot(f);
				ASTToCFGConverter conv = new ASTToCFGConverter();
				CFG cfg = conv.convert(f);

				CFGToUDGConverter udgConv = new CFGToUDGConverter();
				UseDefGraph udg = udgConv.convert(cfg);

				CFGAndUDGToDefUseCFG useDefCfgCreator = new CFGAndUDGToDefUseCFG();
				DefUseCFG defUseCfg = useDefCfgCreator.convert(cfg, udg);
				gv.cfgToDot(cfg, f, null);

				DDGCreator ddgCreator = new DDGCreator();
				DDG ddg = ddgCreator.createForDefUseCFG(defUseCfg);
				
				JoernUtil.printDdg(ddg);
				
				CDGCreator cdgc = new CDGCreator();
				CDG cdg = cdgc.create(cfg);

				//					for (DefUseRelation e : ddg.getDefUseEdges()){
				//						System.out.println(e);
				//						System.out.println(e.src);
				//						System.out.println(e.dst);
				//						System.out.println(e.symbol);
				//						System.out.println();
				//					}

				//					CFGWalker w = new CFGWalker(cfg);
				//					//It already points to the first node of the CFG
				//					System.out.println(w.getCurrentNode());
				//					
				//					while(w.hasNext()){
				//						w.moveToNext();
				//						System.out.println(w.getCurrentNode());
				//					}
				//w.visitAll();

//				System.out.println("BASIC BLOCKS: ");
//				BasicBlocksExtractor h = new BasicBlocksExtractor();
//				List<BasicBlock> basicBlocks = h.getBasicBlocks((CCFG) cfg, cdg);
//				System.out.println(basicBlocks);
//				System.out.println();
				
//				ConditionsConstraintsExtractor e = new ConditionsConstraintsExtractor((CCFG) cfg);
//				e.extract();
			}

			Files.delete(Paths.get(Preprocessed));
		} catch (Exception e) {
			e.printStackTrace();
		}

		//	private static void visitAST(ASTNode node, ASTNodeVisitor visitor){
		//		visitor.defaultHandler(node);
		//		for (int i = 0; i < node.getChildCount(); i++)
		//			visitAST(node.getChild(i), visitor);
		//	}
	}

}

