@AGENTS.md

# Working method for Claude Code

You are the main agent (Sonnet). You plan, decide and do the ordinary work yourself.
Only use subagents when that saves tokens or really improves quality. Every subagent
starts with an empty context, reloads these files and reads code again; that overhead has to be worth it.

## Agents

- scout (Haiku): read-only searching and inventories. Use it instead of the built-in Explore agent.
- builder (Sonnet): larger, self-contained implementation tasks, and writing AGENTS.md.
- reviewer (Opus 5.5): hard bugs, architecture, correctness reviews. Changes nothing itself. The most expensive model: use sparingly.

## Which agent when

- Do it yourself: answering questions, reading one or a few files, changes in a few files
  where you already know where they go, small bug fixes with a clear cause.
- Scout: only for broad searching, such as exploring unfamiliar code, looking for something across many files
  or making an inventory. Not for a file you can open yourself in one step.
- Builder: only for a larger sub-task that stands on its own (many files or many steps),
  or for several independent sub-tasks that can run at the same time. It is the same Sonnet as you, so
  delegating does not give a cheaper model, only a cleaner context.
- Reviewer: only with a clear trigger:
  - security, authentication or handling of secrets;
  - data migrations or changes that can corrupt data;
  - concurrency, caching or other logic where a mistake is expensive and hard to find;
  - an architecture choice that is hard to undo;
  - a bug whose cause is still unclear after two attempts.
  Not for ordinary changes, formatting, documentation or small fixes.

## AGENTS.md

AGENTS.md holds the project knowledge: what the project does, the structure, commands and conventions.
It is tool-neutral, so that other AI tools can read it too. This CLAUDE.md only holds the working method for Claude Code.

At the start of every session:
1. Check whether AGENTS.md exists and whether it still contains "NOT FILLED IN YET".
2. If so, and the project already contains code: have the scout take an inventory of the project (purpose, folder structure,
   languages and dependencies, commands for building, testing and running, existing conventions). Then have the
   builder fill in AGENTS.md based on that report, following the layout already in the file, and remove the
   marker. If the project is new or almost empty, fill in AGENTS.md yourself.
3. Put any ambiguities the scout could not resolve to the user, instead of guessing.

Keep AGENTS.md up to date. If anything changes in structure, commands, dependencies or conventions,
update AGENTS.md in the same task. Do not put a session log or task list in it.

## Order of work for larger tasks

Small tasks: just do them, check and report. For larger tasks:

1. Explore: read what you need yourself; only involve the scout if the search is broad.
2. Plan: split the task into self-contained sub-tasks with acceptance criteria.
   If it concerns an architecture choice or a bug with an unclear cause, first ask the reviewer for analysis and advice.
3. Build: do it yourself, or give the builder a self-contained assignment per sub-task with file paths,
   requirements and what it must report. Subagents do not see this conversation; put everything needed in the assignment.
4. Check: run the tests yourself or verify the result. Only have the reviewer look along
   for one of the triggers above.
5. Repair: fix points of criticism yourself or send them back to the builder. After two failed attempts:
   ask the reviewer to analyse the cause.
6. Finish: tell the user what was done, which agents you used, and how it was checked.
