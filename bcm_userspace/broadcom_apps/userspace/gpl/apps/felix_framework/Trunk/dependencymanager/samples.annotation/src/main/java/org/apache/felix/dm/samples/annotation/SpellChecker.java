/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package org.apache.felix.dm.samples.annotation;

import java.util.concurrent.CopyOnWriteArrayList;

import org.apache.felix.dm.annotation.api.Property;
import org.apache.felix.dm.annotation.api.Service;
import org.apache.felix.dm.annotation.api.ServiceDependency;
import org.apache.felix.dm.annotation.api.Start;
import org.apache.felix.dm.annotation.api.Stop;
import org.apache.felix.service.command.CommandProcessor;
import org.apache.felix.service.command.Descriptor;
import org.osgi.service.log.LogService;

/**
 * Felix "spellcheck" Gogo Shell Command. This command allows to check if some given words are valid or not.
 * This command will be activated only if (at least) one DictionaryService has been injected.
 * To create a Dictionary Service, you have to go the the web console and define a "Dictionary Services" factory
 * configuration instance, which will fire an instantiation of the corresponding dictionary service.
 */
@Service(provide={SpellChecker.class}, 
         properties={@Property(name=CommandProcessor.COMMAND_SCOPE, value="dmsample.annotation"),
                     @Property(name=CommandProcessor.COMMAND_FUNCTION, values={"spellcheck"})})
public class SpellChecker
{
    /**
     * We'll use the OSGi log service for logging. If no log service is available, then we'll use a NullObject.
     */
    @ServiceDependency(required = false)
    private LogService m_log;

    /**
     * We'll store all Dictionaries is a CopyOnWrite list, in order to avoid method synchronization.
     */
    private CopyOnWriteArrayList<DictionaryService> m_dictionaries = new CopyOnWriteArrayList<DictionaryService>();

    /**
     * Inject a dictionary into this service.
     * @param serviceProperties the dictionary OSGi service properties
     * @param dictionary the new dictionary
     */
    @ServiceDependency(removed = "removeDictionary")
    protected void addDictionary(DictionaryService dictionary)
    {
        m_dictionaries.add(dictionary);
    }
    
    /**
     * Lifecycle method callback, used to check if our service has been activated.
     */
    @Start
    protected void start() {
        m_log.log(LogService.LOG_WARNING, "Spell Checker started");
    }
    
    /**
     * Lifecycle method callback, used to check if our service has been activated.
     */
    @Stop
    protected void stop() {
        m_log.log(LogService.LOG_WARNING, "Spell Checker stopped");
    }
        
    /**
     * Remove a dictionary from our service.
     * @param dictionary
     */
    protected void removeDictionary(DictionaryService dictionary)
    {
        m_dictionaries.remove(dictionary);
    }

    // --- Gogo Shell command

    @Descriptor("checks if word is found from an available dictionary")
    public void spellcheck(@Descriptor("the word to check")String word)
    {
        m_log.log(LogService.LOG_DEBUG, "Checking spelling of word \"" + word
            + "\" using the following dictionaries: " + m_dictionaries);

        for (DictionaryService dictionary : m_dictionaries)
        {
            if (dictionary.checkWord(word))
            {
                System.out.println("word " + word + " is correct");
                return;
            }
        }
        System.err.println("word " + word + " is incorrect");
    }
}
