/**
 * 
 */
package tools.safepatch.cfgclassic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import ast.ASTNode;
import ast.statements.Condition;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;

/**
 * Class representing map between basicblock of the old cfg and
 * basicblock of the new cfg.
 * @author machiry
 *
 */
public class BasicBlockMap {
	
	private BasicBlock oldBasicBlock = null;
	private BasicBlock newBasicBlock = null;
	
	private MAP_TYPE mapType = MAP_TYPE.UNDEFINED;
	
	private List<StatementMap> targetStatementMap = new ArrayList<StatementMap>(); 
	
	public BasicBlockMap(BasicBlock oldBB, BasicBlock newBB, MAP_TYPE currM) {
		this.oldBasicBlock = oldBB;
		this.newBasicBlock = newBB;
		this.mapType = currM;
	}
	

	
	/***
	 * 
	 * @param currStMap
	 * @param existingMaps
	 * @param oldBBList
	 * @param newBBList
	 * @return
	 */
	public static BasicBlockMap getorCreateMap(StatementMap currStMap, List<BasicBlockMap> existingMaps, List<BasicBlock> oldBBList, List<BasicBlock> newBBList) {
		BasicBlock oldBB = null;
		BasicBlock newBB = null;
		// get old basicblock
		if(currStMap.getOriginal() != null) {
			for(BasicBlock currBB:oldBBList) {
				if(currBB.containsNode(currStMap.getOriginal())) {
					oldBB = currBB;
					break;
				}
			}
		}
		// get new basicblock
		if(currStMap.getNew() != null) {
			for(BasicBlock currBB:newBBList) {
				if(currBB.containsNode(currStMap.getNew())) {
					newBB = currBB;
					break;
				}
			}
		}
		
		BasicBlockMap newMap = new BasicBlockMap(oldBB, newBB, currStMap.getMapType());
		if(existingMaps.contains(newMap)) {
			int currIn = existingMaps.indexOf(newMap);
			newMap = existingMaps.get(currIn);
		} else {
			existingMaps.add(newMap);
		}
		newMap.addStatementMap(currStMap);
		return newMap;
		
	}
	
	public void addStatementMap(StatementMap currMap) {
		if(!this.targetStatementMap.contains(currMap)) {
			this.targetStatementMap.add(currMap);
		}
	}
	
	public boolean hasBB(BasicBlock targetBB) {
		if(targetBB != null) {
			return this.oldBasicBlock == targetBB || this.newBasicBlock == targetBB;
		}
		return false;
	}
	
	public BasicBlock getOld() {
		return this.oldBasicBlock;
	}
	
	public BasicBlock getNew() {
		return this.newBasicBlock;
	}
	
	public HashMap<ASTNode, ASTNode> getMappedConditions() {
		HashMap<ASTNode, ASTNode> retVal = new HashMap<ASTNode, ASTNode>();
		if(this.mapType == MAP_TYPE.UNMODIFIED || this.mapType == MAP_TYPE.MOVE || this.mapType == MAP_TYPE.UPDATE) {
			for(StatementMap currM:this.targetStatementMap) {
				ASTNode oldA = currM.getOriginal();
				ASTNode newA = currM.getNew();
				if(oldA != null && newA != null) {
					if(oldA instanceof Condition && newA instanceof Condition) {
						retVal.put(oldA, newA);
					}
				}
			}
		}		
		return retVal;
	}
	
	public List<ASTNode> getNewConditions() {
		ArrayList<ASTNode> retVal = new ArrayList<ASTNode>();
		if(this.mapType == MAP_TYPE.INSERT) {
			for(StatementMap currM:this.targetStatementMap) {
				ASTNode newA = currM.getNew();
				if(newA != null) {
					if(newA instanceof Condition) {
						retVal.add(newA);
					}
				}
			}
		}	
		return retVal;
	}
	
	@Override
	public int hashCode() {
		int retVal = 0;
		if(this.oldBasicBlock != null) {
			retVal ^= this.oldBasicBlock.hashCode();
		}
		if(this.newBasicBlock != null) {
			retVal ^= this.newBasicBlock.hashCode();
		}
		return retVal;
	}
	
	
	@Override
	public boolean equals(Object o) {
		boolean retVal = false;
		if(o instanceof BasicBlockMap) {
			BasicBlockMap that = (BasicBlockMap)o;
			if(this.oldBasicBlock == null) {
				retVal = this.oldBasicBlock == that.oldBasicBlock;
			} else {
				retVal = this.oldBasicBlock.equals(that.oldBasicBlock);
			}
			if(this.newBasicBlock == null) {
				retVal &= this.newBasicBlock == that.newBasicBlock;
			} else {
				retVal &= this.newBasicBlock.equals(that.newBasicBlock);
			}
		}
		return retVal;
	}
	
}
