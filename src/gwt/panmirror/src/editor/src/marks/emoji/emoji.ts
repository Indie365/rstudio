/*
 * emoji.ts
 *
 * Copyright (C) 2020 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

import { Schema, Mark, Fragment, Node as ProsemirrorNode } from 'prosemirror-model';
import { InputRule } from 'prosemirror-inputrules';
import { EditorState, Transaction } from 'prosemirror-state';

import { Extension } from '../../api/extension';
import { PandocOutput, PandocToken, PandocTokenType, ProsemirrorWriter, PandocExtensions } from '../../api/pandoc';
import { pandocAttrReadAST } from '../../api/pandoc_attr';
import { fragmentText } from '../../api/fragment';

import { FixupContext } from '../../api/fixup';
import { MarkTransaction } from '../../api/transaction';
import { mergedTextNodes } from '../../api/text';
import { emojis, emojiFromAlias, emojiFromChar, emojiForAllSkinTones, EmojiWithSkinTone, emojiWithSkinTonePreference, emojiFromString } from '../../api/emoji';
import { emojiCompletionHandler, emojiSkintonePreferenceCompletionHandler } from './emoji-completion';
import { getMarkAttrs } from '../../api/mark';
import { PandocCapabilities } from '../../api/pandoc_capabilities';
import { EditorUI } from '../../api/ui';

const kEmojiAttr = 0;
const kEmojiContent = 1;

const extension = (_exts: PandocExtensions, _caps: PandocCapabilities, ui: EditorUI): Extension | null => {
  return {
    marks: [
      {
        name: 'emoji',
        spec: {
          inclusive: false,
          noInputRules: true,
          attrs: {
            emojihint: {},
          },
          parseDOM: [
            {
              tag: "span[class*='emoji']",
              getAttrs(dom: Node | string) {
                const el = dom as Element;
                return {
                  emojihint: el.getAttribute('data-emojihint'),
                };
              },
            },
          ],
          toDOM(mark: Mark) {
            return [
              'span',
              {
                class: 'emoji',
                title: ':' + mark.attrs.emojihint + ':',
                'data-emojihint': mark.attrs.emojihint,
              },
            ];
          },
        },
        pandoc: {
          readers: [
            {
              token: PandocTokenType.Span,
              match: (tok: PandocToken) => {
                const attrs = pandocAttrReadAST(tok, kEmojiAttr);
                return attrs.keyvalue.length > 0 && attrs.keyvalue[0][0] === 'data-emoji';
              },
              handler: (schema: Schema) => (writer: ProsemirrorWriter, tok: PandocToken) => {
                const attrs = pandocAttrReadAST(tok, kEmojiAttr);
                const emojihint = attrs.keyvalue[0][1];
                const emojiMark = schema.marks.emoji.create({ emojihint });
                writer.openMark(emojiMark);
                const emojiChar = tok.c[kEmojiContent][0].c;
                writer.writeText(emojiChar);
                writer.closeMark(emojiMark);
              },
            },
          ],
          writer: {
            priority: 16,
            write: (output: PandocOutput, mark: Mark, parent: Fragment) => {
              // look for a matching emoji
              const char = fragmentText(parent);
              const emoji = emojiFromChar(char);
              if (emoji) {
                output.writeToken(PandocTokenType.Span, () => {
                  // resolve which alias to use
                  let alias = emoji.aliases[0];
                  if (emoji.aliases.length > 1) {
                    if (emoji.aliases.includes(mark.attrs.emojihint)) {
                      alias = mark.attrs.emojihint;
                    }
                  }
                  output.writeAttr('', ['emoji'], [['data-emoji', alias]]);
                  output.writeArray(() => {
                    output.writeText(emoji.emoji);
                  });
                });
              } else {
                output.writeInlines(parent);
              }
            },
          },
        },
      },
    ],

    inputRules: () => {
      return [
        new InputRule(/(^|[^`]):(\w+):$/, (state: EditorState, match: string[], start: number, end: number) => {
          const emojiName = match[2];
          const emoji = emojiFromAlias(emojiName);
          if (emoji) {
            const emojiWithSkinTone = emojiWithSkinTonePreference(emoji, ui.prefs.emojiSkinTone());
            const schema = state.schema;
            const tr = state.tr;
            tr.delete(start + match[1].length, end);
            const mark = schema.marks.emoji.create({ emojihint: emojiName });
            const text = schema.text(emojiWithSkinTone.emojiWithSkinTone, [mark]);
            tr.replaceSelectionWith(text, false);
            return tr;
          } else {
            return null;
          }
        }),
      ];
    },

    completionHandlers: () => [emojiCompletionHandler(ui), emojiSkintonePreferenceCompletionHandler(ui)],

    fixups: (schema: Schema) => {
      
      return [
        (tr: Transaction, context: FixupContext) => {
          // only apply on save and load
          if (![FixupContext.Save, FixupContext.Load].includes(context)) {
            return tr;
          }

          // create mark transation wrapper
          const markTr = new MarkTransaction(tr);

          const textNodes = mergedTextNodes(markTr.doc, (_node: ProsemirrorNode, parentNode: ProsemirrorNode) =>
            parentNode.type.allowsMarkType(schema.marks.emoji),
          );

          
          textNodes.forEach(textNode => {
            // Since emoji can be composed of multiple characters (including
            // other emoji), we always need to prefer the longest match when inserting
            // a mark for any given starting position.

            // Find the possible emoji at each position in this text node
            const possibleMarks = new Map<number, Array<{to: number, emoji: EmojiWithSkinTone}>>();
            for (const emoji of emojis(ui.prefs.emojiSkinTone())) {
              emojiForAllSkinTones(emoji).forEach(skinToneEmoji =>
                {
                  const charLoc = textNode.text.indexOf(skinToneEmoji.emojiWithSkinTone);
                  if (charLoc !== -1) {
                    const from = textNode.pos + charLoc;
                    const to = from + skinToneEmoji.emojiWithSkinTone.length;
                    possibleMarks.set(
                      from,
                      (possibleMarks.get(from) || []).concat({to, emoji: skinToneEmoji})
                    );
                  }
                });
            }

            // For each position that has emoji, use the longest emoji match as the
            // emoji to be marked. 
            possibleMarks.forEach((possibleEmojis, markFrom) => {
              
              const orderedEmojis = possibleEmojis.sort((a,b) => b.to - a.to);
              const to = orderedEmojis[0].to;
              const emoji = orderedEmojis[0].emoji;

              // remove any existing mark (preserving attribues if we do )
              let existingAttrs: { [key: string]: any } | null = null;
              if (markTr.doc.rangeHasMark(markFrom, to, schema.marks.emoji)) {
                existingAttrs = getMarkAttrs(markTr.doc, { from: markFrom, to }, schema.marks.emoji );
                markTr.removeMark(markFrom, to, schema.marks.emoji);
              }

              // create a new mark
              const mark = schema.marks.emoji.create({ 
                emojihint: emoji.aliases[0],
                ...existingAttrs
              });

              // on load we want to cover the entire span
              if (context === FixupContext.Load) {
                markTr.addMark(markFrom, to, mark);
              // on save we just want the raw emjoi character(s)
              } else if (context === FixupContext.Save) {
                markTr.addMark(markFrom, markFrom + emoji.emoji.length, mark);
              }
            });          
          });

          return tr;
        },
      ];
    },
  };
};

export default extension;
