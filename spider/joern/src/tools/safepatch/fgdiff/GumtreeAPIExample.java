package tools.safepatch.fgdiff;

import java.io.IOException;
import java.util.List;

import com.github.gumtreediff.actions.ActionGenerator;
import com.github.gumtreediff.actions.model.Action;
import com.github.gumtreediff.client.Run;
import com.github.gumtreediff.gen.Generators;
import com.github.gumtreediff.gen.jdt.JdtTreeGenerator;
import com.github.gumtreediff.matchers.MappingStore;
import com.github.gumtreediff.matchers.Matcher;
import com.github.gumtreediff.matchers.Matchers;
import com.github.gumtreediff.tree.ITree;
import com.github.gumtreediff.tree.TreeContext;

/**
 * @see https://github.com/GumTreeDiff/gumtree/wiki/GumTree-API
 * @author Eruc Camellini
 */

public class GumtreeAPIExample {
	
	public static void main(String[] args) {
		Run.initGenerators();
		String file_old = "../test_files/json_example_1/old.c";
		String file_new = "../test_files/json_example_1/new.c";
		
		/*
		 * TRY TO CREATE THE TREE FROM JOERN:
		 * CHECK TreeIoUtils generate method
		 * 
		 */
		try {
			
//			TreeContext tc;
//			tc = Generators.getInstance().getTree(file_old); // retrieve the default generator for the file
//			ITree t = tc.getRoot(); // return the root of the tree
//			System.out.println(t.toTreeString());
			
			ITree src = Generators.getInstance().getTree(file_old).getRoot();
			ITree dst = Generators.getInstance().getTree(file_new).getRoot();
			Matcher m = Matchers.getInstance().getMatcher(src, dst); // retrieve the default matcher
			m.match();
			MappingStore mappings = m.getMappings(); // return the mapping store
			ActionGenerator g = new ActionGenerator(src, dst, mappings);
			g.generate();
			List<Action> actions = g.getActions(); // return the actions
			for (Action action : actions){
				System.out.println(action.toString());
			}
				
		} catch (UnsupportedOperationException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} 
		
		
		
	}
}
