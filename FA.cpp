#include <ctype.h>
#include <set>
#include "interfaces.h"

bool FA::evalByOperatorType(RegexOperator* pRegOperator, stack<RegexOperand*>& operands)
{
	if (pRegOperator == NULL || operands.empty())
	{
		return false;
	}

	vector<RegexOperand*> pVec;
	RegexOperand* pOp1 = NULL;
	RegexOperand* pOp2 = NULL;

	switch (pRegOperator->GetOperandNum())
	{
	case 2:

		if (operands.size() < 2)
		{
			return false;
		}

		pOp1 = operands.top();
		operands.pop();
		pOp2 = operands.top();
		operands.pop();
		pVec.push_back(pOp1);
		pVec.push_back(pOp2);
		operands.push((RegexOperand*)pRegOperator->eval(this, pVec));
		delete pOp1;
		delete pOp2;
		break;

	case 1:
		pOp1 = operands.top();
		operands.pop();
		pVec.push_back(pOp1);
		operands.push((RegexOperand*)pRegOperator->eval(this, pVec));
		delete pOp1;
		break;
	default:
		return false;
	}

	return true;
}

// eval str value
// return true on success and set outputValue
bool FA::eval()
{
	string processedStr = _normalizedExp;
	stack<RegexOperator*> operators;
	stack<RegexOperand*> operands;

	for (unsigned int i = 0; i < processedStr.size(); i++)
	{
		if (isSupportOperand(processedStr[i]))
		{
			operands.push(new RegexOperand(createSingleCharState(processedStr[i])));
		}
		else
		{
			RegexOperator* pRegexOperator = RegexOperatorFactory::GetRegexObject(processedStr[i]);

			if (NULL != pRegexOperator)
			{
				if (pRegexOperator->GetType() == RegexLeftParenthesesType)
				{
					operators.push(pRegexOperator);
				}
				else if (pRegexOperator->GetType() == RegexRightParenthesesType)
				{
					if (operators.empty())
					{
						return false;
					}

					RegexOperator* pTmpRegexOperator = operators.top();
					operators.pop();

					while (pTmpRegexOperator->GetType() != RegexLeftParenthesesType)
					{
						if (!evalByOperatorType(pTmpRegexOperator, operands))
						{
							return false;
						}

						delete pTmpRegexOperator;

						if (operators.empty())
						{
							return false;
						}
						else
						{
							pTmpRegexOperator = operators.top();
							operators.pop();
						}
					}

					// assert GetType() == RegexLeftParenthesesType
					delete pTmpRegexOperator;
				}
				else
				{
					RegexOperator* pTmpRegexOperator = NULL;

					while (!operators.empty() && (pTmpRegexOperator = operators.top())->GetPriority() >= pRegexOperator->GetPriority())
					{
						if (!evalByOperatorType(pTmpRegexOperator, operands))
						{
							return false;
						}

						operators.pop();
						delete pTmpRegexOperator;
					}

					operators.push(pRegexOperator);
				}
			}
			else
			{
				return false;
			}
		}

	}

	if ((!operators.empty() && ((RegexOperator*)operators.top())->GetType() != RegexEndType)
		|| operands.size() != 1)
	{
		return false;
	}

	RegexOperand* rgOperand = operands.top();
	this->_startState = rgOperand->GetValue().startState;
	this->_endState.push_back(rgOperand->GetValue().endState);
	delete rgOperand;
	operands.pop();

	return true;
}

FA::State FA::createState()
{
	State newState = _allState.size();
	_allState.push_back(newState);
	return newState;
}

bool FA::containsEndStates(vector<State> states)
{
	for (unsigned int i = 0; i < states.size(); i++)
	{
		for (unsigned int j = 0; j < _endState.size(); j++)
		{
			if (states[i] == _endState[j])
			{
				return true;
			}
		}
	}

	return false;
}

bool FA::isInEndStates(State state)
{
	for (unsigned int j = 0; j < _endState.size(); j++)
	{
		if (state == _endState[j])
		{
			return true;
		}
	}

	return false;
}

bool FA::isSupportOperand(char c)
{
	if (RegexOperatorFactory::GetRegexObject(c) != NULL)
	{
		return false;
	}
	else if (isascii(c))
	{
		return true;
	}
	else
	{
		return false;
	}
}

///preproess to parse for regex string
// 1 add "&" for AND regex operation
// 2 add "#" for Regex End operation
void FA::preprocess()
{
	string processedStr;
	string str = _exp;

	if (str.empty())
	{
		return;
	}

	// step1: add operator '&'
	int indexP = -1;

	// to do: refactor code to encapsulate variants
	for (unsigned int i = 0; i < str.size(); i++)
	{
		if (indexP != -1 && (
			((isSupportOperand(str[indexP])
			|| RegexOperatorFactory::GetRegexObject(str[indexP])->GetType() == RegexStarOperatorType
			|| RegexOperatorFactory::GetRegexObject(str[indexP])->GetType() == RegexQuestionOperatorType
			|| RegexOperatorFactory::GetRegexObject(str[indexP])->GetType() == RegexRightParenthesesType)
			&& isSupportOperand(str[i]))
			|| (isSupportOperand(str[indexP]) && RegexOperatorFactory::GetRegexObject(str[i])->GetType() == RegexLeftParenthesesType)))
		{
			processedStr += '&';
		}

		processedStr += str[i];
		indexP = i;
	}

	//step2: add '#' as sentinel
	processedStr += '#';

	_normalizedExp = processedStr;
}


FA::SubGraphInfo FA::performANDOperator(SubGraphInfo dest, SubGraphInfo src)
{
	createTransite(src.endState, kNULLTransite, dest.startState);

	SubGraphInfo info;
	info.startState = src.startState;
	info.endState = dest.endState;
	return info;
}

FA::SubGraphInfo FA::performOROperator(SubGraphInfo dest, SubGraphInfo src)
{
	State startState = createState();
	createTransite(startState, kNULLTransite, dest.startState);
	createTransite(startState, kNULLTransite, src.startState);

	State endState = createState();
	createTransite(src.endState, kNULLTransite, endState);
	createTransite(dest.endState, kNULLTransite, endState);

	SubGraphInfo info;
	info.startState = startState;
	info.endState = endState;

	return info;
}

FA::SubGraphInfo FA::performSTAROperator(SubGraphInfo a)
{
	createTransite(a.endState, kNULLTransite, a.startState);

	State startState = createState();
	State endState = createState();

	createTransite(startState, kNULLTransite, a.startState);
	createTransite(startState, kNULLTransite, endState);
	createTransite(a.endState, kNULLTransite, endState);

	SubGraphInfo info;
	info.startState = startState;
	info.endState = endState;
	return info;
}

FA::SubGraphInfo FA::performQuestionOperator(SubGraphInfo a)
{
	State startState = createState();
	State endState = createState();

	createTransite(startState, kNULLTransite, a.startState);
	createTransite(startState, kNULLTransite, endState);
	createTransite(a.endState, kNULLTransite, endState);

	SubGraphInfo info;
	info.startState = startState;
	info.endState = endState;
	return info;
}

FA::SubGraphInfo FA::createSingleCharState(char a)
{
	State startState = createState();
	State endState = createState();
	createTransite(startState, a, endState);

	SubGraphInfo info;
	info.startState = startState;
	info.endState = endState;

	return info;
}

bool FA::createTransite(State startState, char c, State endState)
{
	while (startState >= _transitionTable.size())
	{
		map<char, vector<State>> tmpMap;
		_transitionTable.push_back(tmpMap);
	}

	if (_transitionTable[startState].find(c) != _transitionTable[startState].end())
	{
		_transitionTable[startState][c].push_back(endState);
	}
	else
	{
		vector<State> endStates;
		endStates.push_back(endState);
		_transitionTable[startState].insert(std::pair<char, vector<State>>(c, endStates));
	}

	return true;
}

vector<string> FA::extractMatchStrings(string strToExtract)
{
	vector<string> result;

	unsigned int index = 0;

	while (index < strToExtract.length())
	{
		int matchedIndex = FindFirstMatchedString(strToExtract.substr(index, strToExtract.length() - index));

		if (matchedIndex > 0)
		{
			result.push_back(strToExtract.substr(index, matchedIndex));
			index = index + matchedIndex;
		}
		else
		{
			index++; //to do: try to improve perf here
		}
	}

	return result;
}

// Accept means strToParse whole exact string will be accepted
bool FA::isAccept(string strToParse)
{
	int index = FindFirstMatchedString(strToParse);

	if (index == strToParse.length())
	{
		return true;
	}
	else
	{
		return false;
	}
}

vector<FA::State> FA::GetStatesFromTransitionTable(State s, char c)
{
	vector<State> results;

	if (_transitionTable.size() <= s)
	{
		return results;
	}

	set<State> resultSet;

	if (_transitionTable[s].find(c) != _transitionTable[s].end())
	{
		resultSet.insert(_transitionTable[s][c].begin(), _transitionTable[s][c].end());
	}

	if (c != kNULLTransite)
	{
		if (_transitionTable[s].find(kAnyCharMatch) != _transitionTable[s].end())
		{
			resultSet.insert(_transitionTable[s][kAnyCharMatch].begin(), _transitionTable[s][kAnyCharMatch].end());
		}
	}
	
	// to do: investigate set & vector copy method to avoid initialization here
	vector<State> finalResults(resultSet.begin(), resultSet.end());

	return finalResults;
}