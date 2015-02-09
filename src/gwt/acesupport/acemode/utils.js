/*
 * utils.js
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
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

define("mode/utils", function(require, exports, module) {

(function() {

   var that = this;

   this.contains = function(array, object)
   {
      for (var i = 0; i < array.length; i++)
         if (array[i] === object)
            return true;

      return false;
   };

   this.isArray = function(object)
   {
      return Object.prototype.toString.call(object) === '[object Array]';
   };

   this.getPrimaryState = function(session, row, state)
   {
      var result = session.getState(row);
      if (that.isArray(result))
      {
         return result[0];
      }
      return result;
   };

   this.primaryState = function(states)
   {
      if (that.isArray(states))
         return states[0];
      return states;
   };
   
}).call(exports);

});
