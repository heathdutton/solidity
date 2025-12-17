#include <libyul/backends/evm/ssa/OperationForwardShuffler.h>

#include <boost/test/unit_test.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace
{
using Liveness = solidity::yul::ssa::LivenessAnalysis::LivenessData;
using Slot = solidity::yul::ssa::StackSlot;
using ValueId = solidity::yul::ssa::SSACFG::ValueId;

/// Parse a value ID token like "v172", "phi109", "lit7"
/// Returns std::nullopt for "JUNK"
std::optional<ValueId> parseValueToken(std::string const& token)
{
	if (token == "JUNK")
		return std::nullopt;

	if (token.starts_with("v"))
	{
		size_t num = std::stoull(token.substr(1));
		return ValueId::makeVariable(num);
	}

	if (token.starts_with("phi"))
	{
		size_t num = std::stoull(token.substr(3));
		return ValueId::makePhi(num);
	}

	if (token.starts_with("lit"))
	{
		size_t num = std::stoull(token.substr(3));
		return ValueId::makeLiteral(num);
	}
	throw std::runtime_error("Unknown token: " + token);
}

/// Parse a string like "[v172, phi109, lit7, JUNK]" into Stack::Data
std::vector<Slot> parseStackData(std::string_view _input)
{
	std::vector<Slot> result;
	std::string input(_input);

	// Remove whitespace
	input.erase(std::ranges::remove_if(input, ::isspace).begin(), input.end());

	// Remove brackets
	if (!input.empty() && input.front() == '[')
		input.erase(input.begin());
	if (!input.empty() && input.back() == ']')
		input.pop_back();

	// Split by comma
	std::stringstream ss(input);
	std::string token;

	while (std::getline(ss, token, ','))
	{
		if (token.empty())
			continue;

		if (auto valueId = parseValueToken(token))
			result.push_back(Slot::makeValueID(*valueId));
		else
			result.push_back(Slot::makeJunk());
	}

	return result;
}

/// Parse liveness like "[phi109, phi150, v172]"
/// Returns Liveness with reference count 1 for each value
Liveness parseLiveness(std::string_view _input)
{
	std::vector<std::pair<ValueId, uint32_t>> liveCounts;
	std::string input(_input);

	// Remove whitespace
	input.erase(std::ranges::remove_if(input, ::isspace).begin(), input.end());

	// Remove brackets
	if (!input.empty() && input.front() == '[')
		input.erase(input.begin());
	if (!input.empty() && input.back() == ']')
		input.pop_back();

	// Split by comma
	std::stringstream ss(input);
	std::string token;

	while (std::getline(ss, token, ','))
	{
		if (token.empty())
			continue;

		auto valueId = parseValueToken(token);
		if (valueId)
			liveCounts.emplace_back(*valueId, 1);  // Default reference count of 1
	}

	return Liveness(liveCounts.begin(), liveCounts.end());
}

struct StackManipulationCallbacks
{
	size_t numOps = 0;
	void swap(size_t _depth)
	{
		++numOps;
		std::cout << fmt::format("SWAP {} ", _depth) << std::flush;
		if (hook) (*hook)();
	}
	void dup(size_t _depth)
	{
		++numOps;
		std::cout << fmt::format("DUP {} ", _depth) << std::flush;
		if (hook) (*hook)();
	}
	void push(Slot const& _slot)
	{
		++numOps;
		std::cout << "PUSH " << slotToString(_slot) << std::flush;
		if (hook) (*hook)();
	}
	void pop()
	{
		++numOps;
		std::cout << "POP " << std::flush;
		if (hook) (*hook)();
	}

	std::optional<std::function<void()>> hook = std::nullopt;
};
using Stack = solidity::yul::ssa::Stack<StackManipulationCallbacks>;
}

namespace solidity::yul::test
{
BOOST_AUTO_TEST_SUITE(OperationForwardShufflerTest)

BOOST_AUTO_TEST_CASE(TestCycle)
{
	Stack::Data data = parseStackData("[v64, JUNK, v64, JUNK, v60, v74, JUNK, v60]");
	Stack::Data args = parseStackData("[v74, lit15]");
	Liveness liveness = parseLiveness("[v60, v64]");

	Stack stack(data, {});
	ssa::OperationForwardShuffler<StackManipulationCallbacks>::shuffle(stack, args, liveness, 7, false);
}

BOOST_AUTO_TEST_CASE(TestJunk)
{
	{
		/*// todo extremely inefficient atm
		Stack::Data data = parseStackData("[JUNK, JUNK, v56, v57, JUNK, JUNK]");
		Stack::Data args = parseStackData("[JUNK, JUNK, v56, v57, lit0, v56, lit11]");
		Liveness liveness = parseLiveness("");

		Stack stack(data, {.hook = [&]{ std::cout << " -> " << ssa::stackToString(data) << std::endl; }});
		ssa::OperationForwardShuffler<StackManipulationCallbacks>::shuffle(stack, args, liveness, args.size(), false);*/
	}
	/*{
		Stack::Data data = parseStackData("[JUNK, v44, v129, v43, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, v101, JUNK, v130, v131]");
		Stack::Data args = parseStackData("[JUNK, v44, JUNK, v43, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, JUNK, v101, JUNK, v130, v131, lit0, v129]");
		Liveness liveness = parseLiveness("");

		Stack stack(data, {.hook = [&]{ std::cout << " -> " << ssa::stackToString(data) << std::endl; }});
		ssa::OperationForwardShuffler<StackManipulationCallbacks>::shuffle(stack, args, liveness, args.size(), false);
	}*/
	/*
	 *
	 * add([v6, v5, v4, v3, v2, JUNK, JUNK, v59, v60, JUNK, JUNK, JUNK, JUNK, v98, JUNK, JUNK, JUNK, v113, v114, v115, v116] -> { [v2, v3, v4, v5, v6, v59, v60, v98, v113, v114, v115] } + [v116, v5]) ->
			 -> target size = 20: SWAP2 + SWAP15 + POP + SWAP13 + POP + SWAP9 + POP + POP + SWAP15 + DUP8 + DUP1 + DUP3 + SWAP9 + POP + DUP3 + SWAP8 + POP + DUP3 + SWAP7 + POP + DUP3 + SWAP5 + POP + DUP3 + SWAP4 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + SWAP10 + POP + DUP3 + Unhandled exception during test: /solidity/libyul/backends/evm/ssa/OperationForwardShuffler.h(52): Throw in function static void solidity::yul::ssa::OperationForwardShuffler<Callback, ReachableStackDepth>::shuffle(solidity::yul::ssa::Stack<Callback>&, const std::vector<solidity::yul::ssa::StackSlot>&, const solidity::yul::ssa::LivenessAnalysis::LivenessData&, std::size_t, bool) [with Callback = solidity::yul::ssa::StackLayoutGenerator::StackManipulationCallbacks; long unsigned int ReachableStackDepth = 16; std::size_t = long unsigned int]
Dynamic exception type: boost::wrapexcept<solidity::yul::YulAssertion>
	 *
	 *
	 *  this ends in a cycle but also it does this swap15 which i dont think is great
	 *  instead we should swap up closer to the top if possible so the top region stays intact for the most part
	 *  (reduces entropy)
	 *
	 */
}

BOOST_AUTO_TEST_SUITE_END()
}
