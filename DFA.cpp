#include "interfaces.h"
#include <queue>

bool DFA::buildDFAFromNFA()
{
	if (!_transitionTable.empty())
	{
		return true;
	}

	_dfaNFAStatesMap.clear();

	queue<State> unmarkedStates;
	
	// use NFA with NULL closure states as DFA start state
	_startState = createState();
	vector<State> states;
	states.push_back(_pNFA->_startState);
	vector<State> nfaStartStates = _pNFA->findALLNULLPathStatesFromStates(states);
	_dfaNFAStatesMap.push_back(nfaStartStates);
	unmarkedStates.push(_startState);

	while (!unmarkedStates.empty())
	{
		State unmarkedState = unmarkedStates.front();
		unmarkedStates.pop();

		vector<State> states = _dfaNFAStatesMap[unmarkedState];
		map<char, vector<State>> possibleNFAPathMap;

		// cal from current NFA states, which states path "x" can be reached 
		for (unsigned int i = 0; i < states.size(); i++)
		{
			if (_pNFA->_transitionTable.size() <= states[i])
			{
				continue;
			}

			for (map<char, vector<State>>::iterator itr = _pNFA->_transitionTable[states[i]].begin(); itr != _pNFA->_transitionTable[states[i]].end(); itr++)
			{
				if (possibleNFAPathMap.find(itr->first) != possibleNFAPathMap.end())
				{
					for (unsigned int j = 0; j < itr->second.size(); j++)
					{
						bool isIntheVector = false;

						for (unsigned int k = 0; k < possibleNFAPathMap[itr->first].size(); k++)
						{
							if (possibleNFAPathMap[itr->first][k] == itr->second[j])
							{
								isIntheVector = true;
								break;
							}
						}

						if (!isIntheVector)
						{
							possibleNFAPathMap[itr->first].push_back(itr->second[j]);
						}
					}
				}
				else
				{
					possibleNFAPathMap.insert(std::pair<char, vector<State>>(itr->first, itr->second));
				}
			}
		}

		for (map <char, vector<State>>::iterator itr = possibleNFAPathMap.begin(); itr != possibleNFAPathMap.end(); itr++)
		{
			vector<State> possiblePathStates = _pNFA->findALLNULLPathStatesFromStates(itr->second);
			int index = getDFAStateByNFAStates(possiblePathStates);

			if (index == -1)
			{
				State newDFAState = createState();
				_dfaNFAStatesMap.push_back(possiblePathStates);
				unmarkedStates.push(newDFAState);
				createTransite(unmarkedState, itr->first, newDFAState);
			}
			else
			{
				createTransite(unmarkedState, itr->first, (State)index);
			}

		}
	}

	return true;
}

int DFA::getDFAStateByNFAStates(vector<State> states)
{
	for (vector<vector<State>>::iterator itr = _dfaNFAStatesMap.begin(); itr != _dfaNFAStatesMap.end(); itr++)
	{
		bool isFound = true;

		for (vector<State>::iterator needToCheckItr = states.begin(); needToCheckItr != states.end(); needToCheckItr++)
		{
			bool isInVector = false;

			for (vector<State>::iterator subItr = itr->begin(); subItr != itr->end(); subItr++)
			{
				if (*subItr == *needToCheckItr)
				{
					isInVector = true;
					break;
				}
			}

			isFound = isFound && isInVector;
		}

		if (isFound)
		{
			return itr - _dfaNFAStatesMap.begin();
		}
	}

	return -1;
}