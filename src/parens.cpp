/**
 * @file parens.cpp
 * Adds or removes parens.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "parens.h"

#include "log_rules.h"
#include "unc_tools.h"

using namespace uncrustify;


//! Add an open parenthesis after first and add a close parenthesis before the last
static void add_parens_between(Chunk *first, Chunk *last);

size_t Anzahl = 0;

/**
 * Scans between two parens and adds additional parens if needed.
 * This function is recursive. If it hits another open paren, it'll call itself
 * with the new bounds.
 *
 * Adds optional parens in an IF or SWITCH conditional statement.
 *
 * This basically just checks for a CT_COMPARE that isn't surrounded by parens.
 * The edges for the compare are the open, close and any CT_BOOL tokens.
 *
 * This only handles VERY simple patterns:
 *   (!a && b)         => (!a && b)          -- no change
 *   (a && b == 1)     => (a && (b == 1))
 *   (a == 1 || b > 2) => ((a == 1) || (b > 2))
 *
 * FIXME: we really should bail if we transition between a preprocessor and
 *        a non-preprocessor
 */
static void check_bool_parens(Chunk *popen, Chunk *pclose, int nest);


void do_parens()
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_if_bool");

   if (options::mod_full_paren_if_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (  pc->IsNot(CT_SPAREN_OPEN)
            || (  pc->GetParentType() != CT_IF
               && pc->GetParentType() != CT_ELSEIF
               && pc->GetParentType() != CT_SWITCH))
         {
            continue;
         }
         // Grab the close sparen
         Chunk *pclose = pc->GetNextType(CT_SPAREN_CLOSE, pc->GetLevel(), E_Scope::PREPROC);

         if (pclose->IsNotNullChunk())
         {
            check_bool_parens(pc, pclose, 0);
            pc = pclose;
         }
      }
   }
} // do_parens


void do_parens_assign()                         // Issue #3316
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_assign_bool");

   if (options::mod_full_paren_assign_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (pc->Is(CT_ASSIGN))
         {
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->Text(), pc->GetLevel());
            // look before for a open sparen
            size_t check_level = pc->GetLevel();
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

               //log_pcf_flags(LPARADD, p->GetFlags());
               if (p->TestFlags(PCF_STMT_START))
               {
                  break;
               }

               if (p->Is(CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (p->Is(CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->GetLevel() < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

            if (p->GetParentType() == CT_WHILE)
            {
               continue;
            }
            // Grab the semicolon
            Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->GetLevel(), E_Scope::PREPROC);

            if (semicolon->IsNotNullChunk())
            {
               check_bool_parens(pc, semicolon, 0);
               pc = semicolon;
            }
         }
      }
   }
} // do_parens_assign


void do_parens_return()                         // Issue #3316
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_return_bool");

   if (options::mod_full_paren_return_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (pc->Is(CT_RETURN))
         {
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->Text(), pc->GetLevel());
            // look before for a open sparen
            size_t check_level = pc->GetLevel();
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

               //log_pcf_flags(LPARADD, p->GetFlags());
               if (p->TestFlags(PCF_STMT_START))
               {
                  break;
               }

               if (p->Is(CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (p->Is(CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->GetLevel() < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

            if (p->GetParentType() == CT_WHILE)
            {
               continue;
            }
            // Grab the semicolon
            Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->GetLevel(), E_Scope::PREPROC);

            if (semicolon->IsNotNullChunk())
            {
               check_bool_parens(pc, semicolon, 0);
               pc = semicolon;
            }
         }
      }
   }
} // do_parens_return


static void add_parens_between(Chunk *first, Chunk *last)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LPARADD, "%s(%d): line %zu, between '%s' [lvl is %zu] and '%s' [lvl is %zu]\n",
           __func__, __LINE__, first->GetOrigLine(),
           first->Text(), first->GetLevel(),
           last->Text(), last->GetLevel());

   // Don't do anything if we have a bad sequence, ie "&& )"
   Chunk *first_n = first->GetNextNcNnl();

   if (first_n == last)
   {
      return;
   }
   Anzahl++;
   LOG_FMT(LGUY, "\nAnfang: %zu", Anzahl);
   //prot_the_line(__func__, __LINE__, 2, 0);
   //prot_the_columns(__LINE__, 2);
   //prot_the_OrigCols(__LINE__, 2);
   //LOG_FMT(LGUY, "        if   (    va1  <    0    ||   va2  >    0    )\n");
   //rebuild_the_line(__LINE__, 2);
   Chunk pc;

   pc.SetType(CT_PAREN_OPEN);
   pc.SetOrigLine(first_n->GetOrigLine());
   pc.SetColumn(first_n->GetColumn());                         // Issue #3236
   pc.SetOrigCol(first_n->GetOrigCol());                       // Issue #3236
   pc.SetOrigColEnd(first_n->GetOrigColEnd());                 // Issue #3236
   pc.Str() = "(";
   pc.SetFlags(first_n->GetFlags() & PCF_COPY_FLAGS);
   pc.SetLevel(first_n->GetLevel());
   pc.SetPpLevel(first_n->GetPpLevel());
   pc.SetBraceLevel(first_n->GetBraceLevel());
   //LOG_FMT(LGUY, "insert (");
   pc.CopyAndAddBefore(first_n);

   //prot_the_line(__func__, __LINE__, 2, 0);
   //prot_the_columns(__LINE__, 2);
   //prot_the_OrigCols(__LINE__, 2);
   //LOG_FMT(LGUY, "        if   (    (    va1  <    0    ||   va2  >    0    )\n");
   //rebuild_the_line(__LINE__, 2);
   //LOG_FMT(LGUY, "shift all the tokens in this line to the right  Issue #3236\n");
   // shift all the tokens in this line to the right  Issue #3236
   for (Chunk *temp = first_n; ; temp = temp->GetNext())
   {
      //LOG_FMT(LGUY, "%s(%d): orig line %zu, orig col %zu, Column is %zu, Text is '%s'\n",
      //        __func__, __LINE__,
      //        temp->GetOrigLine(), temp->GetOrigCol(), temp->GetColumn(), temp->Text());
      temp->SetColumn(temp->GetColumn() + 1);                         // Issue #3236
      //LOG_FMT(LGUY, "%s(%d): orig line %zu, orig col %zu, OrigCol is %zu, Text is '%s'\n",
      //        __func__, __LINE__,
      //        temp->GetOrigLine(), temp->GetOrigCol(), temp->GetOrigCol(), temp->Text());
      temp->SetOrigCol(temp->GetOrigCol() + 1);                         // Issue #3236
      temp->SetOrigColEnd(temp->GetOrigColEnd() + 1);                   // Issue #3236

      if (temp->Is(CT_NEWLINE))
      {
         break;
      }
   }

   //prot_the_columns(__LINE__, 2);
   //prot_the_OrigCols(__LINE__, 2);
   //LOG_FMT(LGUY, "        if   (    (    va1 <    0    ||   va2  >    0    )\n");
   //rebuild_the_line(__LINE__, 2);

   Chunk *last_prev = last->GetPrevNcNnl(E_Scope::PREPROC);

   //LOG_FMT(LGUY, "%s(%d): last_prev is '%s', orig line %zu, orig Col %zu, Column is %zu\n",
   //LOG_FMT(LGUY, "%s(%d): last_prev is '%s', orig line %zu, orig Col %zu, Column is %zu\n",
   //LOG_FMT(LGUY, "%s(%d): last_prev is '%s', Column is %zu\n",
   //        __func__, __LINE__,
   //        last_prev->Text(),
   //        last_prev->GetColumn());
   //LOG_FMT(LGUY, "%s(%d): last_prev is '%s', OrigCol is %zu\n",
   //        __func__, __LINE__,
   //        last_prev->Text(),
   //        last_prev->GetOrigCol());

   pc.SetType(CT_PAREN_CLOSE);
   pc.SetOrigLine(last_prev->GetOrigLine());
   pc.SetOrigCol(last_prev->GetOrigCol());
   pc.SetColumn(last_prev->GetColumn() + 1);                         // Issue #3236
   pc.SetOrigCol(last_prev->GetOrigCol() + 1);                       // Issue #3236
   pc.SetOrigColEnd(last_prev->GetOrigColEnd() + 1);                 // Issue #3236
   pc.Str() = ")";
   pc.SetFlags(last_prev->GetFlags() & PCF_COPY_FLAGS);
   pc.SetLevel(last_prev->GetLevel());
   pc.SetPpLevel(last_prev->GetPpLevel());
   pc.SetBraceLevel(last_prev->GetBraceLevel());
   //LOG_FMT(LGUY, "insert )\n");
   pc.CopyAndAddAfter(last_prev);

   //prot_the_line(__func__, __LINE__, 2, 0);
   //prot_the_columns(__LINE__, 2);
   //prot_the_OrigCols(__LINE__, 2);
   //LOG_FMT(LGUY, "        if   (    (    va1 <    0    )    ||   va2  >    0    )\n");
   //rebuild_the_line(__LINE__, 2);
   //LOG_FMT(LGUY, "shift all the tokens in this line to the right  Issue #3236\n");
   // shift all the tokens in this line to the right  Issue #3236
   //for (Chunk *temp = last_prev->GetNext();; temp = temp->GetNext())
   for (Chunk *temp = last; ; temp = temp->GetNext())
   {
      //LOG_FMT(LGUY, "%s(%d): line %zu, Column is %zu, Text is '%s'\n",
      //        __func__, __LINE__,
      //        temp->GetOrigLine(), temp->GetColumn(), temp->Text());
      temp->SetColumn(temp->GetColumn() + 1);                         // Issue #3236
      //LOG_FMT(LGUY, "%s(%d): line %zu, OrigCol is %zu, Text is '%s'\n",
      //        __func__, __LINE__,
      //        temp->GetOrigLine(), temp->GetOrigCol(), temp->Text());
      temp->SetOrigCol(temp->GetOrigCol() + 1);                        // Issue #3236
      temp->SetOrigColEnd(temp->GetOrigColEnd() + 1);                  // Issue #3236

      if (temp->Is(CT_NEWLINE))
      {
         break;
      }
   }

   //prot_the_line(__func__, __LINE__, 2, 0);
   //prot_the_columns(__LINE__, 2);
   //prot_the_OrigCols(__LINE__, 2);
   //LOG_FMT(LGUY, "        if   (    (    va1 <    0    )    ||   va2  >    0    )\n");
   //rebuild_the_line(__LINE__, 2);

   for (Chunk *tmp = first_n;
        tmp != last_prev;
        tmp = tmp->GetNextNcNnl())
   {
      tmp->SetLevel(tmp->GetLevel() + 1);
   }

   last_prev->SetLevel(last_prev->GetLevel() + 1);
} // add_parens_between


static void check_bool_parens(Chunk *popen, Chunk *pclose, int nest)
{
   LOG_FUNC_ENTRY();

   Chunk *ref        = popen;
   bool  hit_compare = false;

   LOG_FMT(LPARADD, "%s(%d): nest is %d, popen on line %zu, orig col is %zu, pclose on line %zu, orig col is %zu, level is %zu\n",
           __func__, __LINE__, nest,
           popen->GetOrigLine(), popen->GetOrigCol(),
           pclose->GetOrigLine(), pclose->GetOrigCol(),
           popen->GetLevel());

   Chunk *pc = popen;

   while (  (pc = pc->GetNextNcNnl())->IsNotNullChunk()
         && pc != pclose)
   {
      if (pc->TestFlags(PCF_IN_PREPROC))
      {
         LOG_FMT(LPARADD2, " -- bail on PP %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->GetType()),
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
         return;
      }

      if (  pc->Is(CT_BOOL)
         || pc->Is(CT_QUESTION)
         || pc->Is(CT_COND_COLON)
         || pc->Is(CT_COMMA))
      {
         LOG_FMT(LPARADD2, " -- %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->GetType()),
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

         if (hit_compare)
         {
            hit_compare = false;

            if (!language_is_set(LANG_CS))
            {
               add_parens_between(ref, pc);
            }
         }
         ref = pc;
      }
      else if (pc->Is(CT_COMPARE))
      {
         LOG_FMT(LPARADD2, " -- compare '%s' at line %zu, orig col is %zu, level is %zu\n",
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
         hit_compare = true;
      }
      else if (pc->IsParenOpen())
      {
         Chunk *next = pc->GetClosingParen();

         if (next->IsNotNullChunk())
         {
            check_bool_parens(pc, next, nest + 1);
            pc = next;
         }
      }
      else if (pc->Is(CT_SEMICOLON))                      // Issue #3236
      {
         ref = pc;
      }
      else if (  pc->Is(CT_BRACE_OPEN)
              || pc->Is(CT_SQUARE_OPEN)
              || pc->Is(CT_ANGLE_OPEN))
      {
         // Skip [], {}, and <>
         pc = pc->GetClosingParen();
      }
   }

   if (  hit_compare
      && ref != popen
      && !language_is_set(LANG_CS))
   {
      add_parens_between(ref, pclose);
   }
} // check_bool_parens
