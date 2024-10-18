#include "Gameplay/Collection/StickCollectionController.h"
#include "Gameplay/Collection/StickCollectionView.h"
#include "Gameplay/Collection/StickCollectionModel.h"
#include "Gameplay/GameplayService.h"
#include "Global/ServiceLocator.h"
#include "Gameplay/Collection/Stick.h"
#include <random>
#include <iostream>

namespace Gameplay
{
	namespace Collection
	{
		using namespace UI::UIElement;
		using namespace Global;
		using namespace Graphics;
		using namespace Sound;

		StickCollectionController::StickCollectionController()
		{
			collection_view = new StickCollectionView();
			collection_model = new StickCollectionModel();

			for (int i = 0; i < collection_model->number_of_elements; i++) sticks.push_back(new Stick(i));
		}

		StickCollectionController::~StickCollectionController()
		{
			destroy();
		}

		void StickCollectionController::initialize()
		{
			collection_view->initialize(this);
			initializeSticks();
			reset();
			sort_state = SortState::NOT_SORTING;
			color_delay = collection_model->initial_color_delay;
		}

		void StickCollectionController::initializeSticks()
		{
			float rectangle_width = calculateStickWidth();

			for (int i = 0; i < collection_model->number_of_elements; i++)
			{
				float rectangle_height = calculateStickHeight(i); //calc height

				sf::Vector2f rectangle_size = sf::Vector2f(rectangle_width, rectangle_height);

				sticks[i]->stick_view->initialize(rectangle_size, sf::Vector2f(0, 0), 0, collection_model->element_color);
			}
		}

		void StickCollectionController::update()
		{
			processSortThreadState();
			collection_view->update();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->update();
		}

		

		void StickCollectionController::render()
		{
			collection_view->render();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->render();
		}

		float StickCollectionController::calculateStickWidth()
		{
			float total_space = static_cast<float>(ServiceLocator::getInstance()->getGraphicService()->getGameWindow()->getSize().x);

			// Calculate total spacing as 10% of the total space
			float total_spacing = collection_model->space_percentage * total_space;

			// Calculate the space between each stick
			float space_between = total_spacing / (collection_model->number_of_elements - 1);
			collection_model->setElementSpacing(space_between);

			// Calculate the remaining space for the rectangles
			float remaining_space = total_space - total_spacing;

			// Calculate the width of each rectangle
			float rectangle_width = remaining_space / collection_model->number_of_elements;

			return rectangle_width;
		}

		float StickCollectionController::calculateStickHeight(int array_pos)
		{
			return (static_cast<float>(array_pos + 1) / collection_model->number_of_elements) * collection_model->max_element_height;
		}

		void StickCollectionController::updateStickPosition()
		{
			for (int i = 0; i < sticks.size(); i++)
			{
				
				float x_position = (i * sticks[i]->stick_view->getSize().x) + ((i) * collection_model->elements_spacing);
				float y_position = collection_model->element_y_position - sticks[i]->stick_view->getSize().y;
			
				sticks[i]->stick_view->setPosition(sf::Vector2f(x_position, y_position));
			}
		}

		void StickCollectionController::updateStickPosition(int i)
		{
		
			float x_position = (i * sticks[i]->stick_view->getSize().x) + ((i)*collection_model->elements_spacing);
			float y_position = collection_model->element_y_position - sticks[i]->stick_view->getSize().y;

			sticks[i]->stick_view->setPosition(sf::Vector2f(x_position, y_position));
			
		}

		void StickCollectionController::shuffleSticks()
		{
			std::random_device device;
			std::mt19937 random_engine(device());

			std::shuffle(sticks.begin(), sticks.end(), random_engine);
			updateStickPosition();
		}

		bool StickCollectionController::compareSticksByData(const Stick* a, const Stick* b) const
		{
			return a->data < b->data;
		}

		void StickCollectionController::processSortThreadState()
		{
			if (sort_thread.joinable() && isCollectionSorted()) {
				sort_thread.join();
				sort_state = SortState::NOT_SORTING;
			}
		}

		void StickCollectionController::processBubbleSort() {
			Sound::SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();

			for (int j = 0; j < sticks.size(); j++) {   // Loop through the sticks array
				if (sort_state == SortState::NOT_SORTING) { break; }   // Check if sorting has stopped or been interrupted

				bool swapped = false;  // To track if a swap was made

				for (int i = 1; i < sticks.size() - j; i++) {    // Loop through the array, reducing the range each pass
					if (sort_state == SortState::NOT_SORTING) { break; }      // Check if sorting has stopped or been interrupted

					// Increment the number of array accesses and comparisons
					number_of_array_access += 2;
					number_of_comparisons++;

					sound->playSound(Sound::SoundType::COMPARE_SFX);      // Play the compare sound effect

					// Set the current sticks to the processing color
					sticks[i - 1]->stick_view->setFillColor(collection_model->processing_element_color);
					sticks[i]->stick_view->setFillColor(collection_model->processing_element_color);

					if (sticks[i - 1]->data > sticks[i]->data) {      // Check if the current stick is greater than the next stick
						// Swap the sticks if necessary
						std::swap(sticks[i - 1], sticks[i]);
						swapped = true;  // Set swapped to true if there was a swap
					}

					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

					// Reset the stick colors
					sticks[i - 1]->stick_view->setFillColor(collection_model->element_color);
					sticks[i]->stick_view->setFillColor(collection_model->element_color);

					updateStickPosition();
				}

				if (sticks.size() - j - 1 >= 0) {    // Set the last sorted stick to the placement position color
					sticks[sticks.size() - j - 1]->stick_view->setFillColor(collection_model->placement_position_element_color);
				}

				if (!swapped) { break; }    // If no swaps were made, the array is already sorted
			}
			setCompletedColor();
		}

		void StickCollectionController::processInsertionSort()
		{
			SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();

			for (int i = 1; i < sticks.size(); ++i)
			{

				if (sort_state == SortState::NOT_SORTING) { break; }

				int j = i - 1;
				Stick* key = sticks[i];
				number_of_array_access++; // Access for key stick


				key->stick_view->setFillColor(collection_model->processing_element_color); // Current key is red

				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

				while (j >= 0 && sticks[j]->data > key->data)
				{

					if (sort_state == SortState::NOT_SORTING) { break; }

					number_of_comparisons++;
					number_of_array_access++;

					sticks[j + 1] = sticks[j];
					number_of_array_access++; // Access for assigning sticks[j] to sticks[j + 1]
					sticks[j + 1]->stick_view->setFillColor(collection_model->processing_element_color); // Mark as being compared
					j--;
					sound->playSound(SoundType::COMPARE_SFX);
					updateStickPosition(); // Visual update

					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

					sticks[j + 2]->stick_view->setFillColor(collection_model->selected_element_color); // Mark as being compared

				}

				sticks[j + 1] = key;
				number_of_array_access++;
				sticks[j + 1]->stick_view->setFillColor(collection_model->temporary_processing_color); // Placed key is green indicating it's sorted
				sound->playSound(SoundType::COMPARE_SFX);
				updateStickPosition(); // Final visual update for this iteration
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
				sticks[j + 1]->stick_view->setFillColor(collection_model->selected_element_color); // Placed key is green indicating it's sorted
			}
			setCompletedColor();

		}

		void StickCollectionController::processSelectionSort()
		{
			Sound::SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();

			for (int j = 0; j < sticks.size(); j++) {  
				if (sort_state == SortState::NOT_SORTING) { break; }  
				int minIndex = j;
				sticks[minIndex]->stick_view->setFillColor(collection_model->processing_element_color);
				for (int i = minIndex; i < sticks.size() ; i++) {    
					if (sort_state == SortState::NOT_SORTING) { break; } 

				
					number_of_array_access += 2;
					number_of_comparisons++;

					sound->playSound(Sound::SoundType::COMPARE_SFX); 

					sticks[i]->stick_view->setFillColor(collection_model->processing_element_color);

					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

					if (sticks[i]->data < sticks[minIndex]->data) {
						sticks[minIndex]->stick_view->setFillColor(collection_model->element_color);
						minIndex = i;
						sticks[minIndex]->stick_view->setFillColor(collection_model->temporary_processing_color);
					}
					else {
						sticks[i]->stick_view->setFillColor(collection_model->element_color);
					}
					
				
				}
				std::swap(sticks[j], sticks[minIndex]);
				number_of_array_access += 2;
				sticks[j]->stick_view->setFillColor(collection_model->placement_position_element_color);
				updateStickPosition();
			}
			setCompletedColor();
		}

		void StickCollectionController::processMergeSort()
		{
			mergeSort(0, sticks.size() - 1);
			setCompletedColor();
		}

		void StickCollectionController::processQuickSort()
		{
			quickSort(0, sticks.size() - 1);
			setCompletedColor();
		}

		void StickCollectionController::processRadixSort()
		{
			radixSort();
			setCompletedColor();
		}

		void StickCollectionController::countSort(int exponent) {
			Sound::SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();
			std::vector<Stick*> ans(sticks.size(),0);
			std::vector<int> count(sticks.size(),0);
			for (int i = 0; i < sticks.size(); i++) {
				sticks[i]->stick_view->setFillColor(collection_model->processing_element_color);
				count[(sticks[i]->data / exponent) % 10]++;
				sound->playSound(Sound::SoundType::COMPARE_SFX);
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
				sticks[i]->stick_view->setFillColor(collection_model->element_color);
				number_of_array_access ++;
			}

			for (int i = 1; i < sticks.size(); i++) {
				count[i] = count[i] + count[i - 1];
			}

			for (int i = sticks.size() - 1; i >= 0; i--) {
				int index = --count[(sticks[i]->data / exponent) % 10];
				sticks[i]->stick_view->setFillColor(collection_model->temporary_processing_color);
				ans[index] = sticks[i];
				number_of_array_access++;
			}
			for (int i = 0; i < sticks.size(); i++) {
				sticks[i] = ans[i];
				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
				updateStickPosition(i);
				sound->playSound(Sound::SoundType::COMPARE_SFX);
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
			}
		}

		void StickCollectionController::radixSort() {
			int maxNumber = sticks[0]->data;
			for (int i = 1; i < sticks.size(); i++) {
				if (maxNumber < sticks[i]->data) {
					maxNumber = sticks[i]->data;
				}
			}
			for (int exponent = 1; maxNumber/exponent >0; exponent *=10) {
				countSort( exponent);
			}
		}



		int StickCollectionController::partition(int low, int high) {

			Sound::SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();
			Stick* pivot = sticks[high];
			sticks[high]->stick_view->setFillColor(collection_model->selected_element_color);
			int swapIndex = low - 1;
			for (int currentIndex = low; currentIndex < high; currentIndex++) {
				number_of_array_access += 2;
				number_of_comparisons++;
				sticks[currentIndex]->stick_view->setFillColor(collection_model->processing_element_color);
				if (sticks[currentIndex]->data <= pivot->data) {
					swapIndex++;
					std::swap(sticks[swapIndex], sticks[currentIndex]);
					updateStickPosition();
					sound->playSound(Sound::SoundType::COMPARE_SFX);
					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
				}
				else {
					sticks[currentIndex]->stick_view->setFillColor(collection_model->element_color);
				}
			}
			std::swap(sticks[swapIndex + 1], sticks[high]);
			number_of_array_access += 2;
			updateStickPosition();
			return swapIndex + 1;
		}

		void StickCollectionController::quickSort( int low, int high) {
			if (low < high) {
				int pivotIndex = partition(low, high);
				quickSort( low, pivotIndex - 1);
				quickSort(pivotIndex + 1, high);
			}
		}

		// Out-of-Place Merge function
		void StickCollectionController::merge(int left, int mid, int right)
		{
			SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();

			std::vector<Stick*> temp(right - left + 1);
			int k = 0;

			// Copy elements to the temporary array
			for (int index = left; index <= right; ++index) {
				temp[k++] = sticks[index];
				number_of_array_access++;
				sticks[index]->stick_view->setFillColor(collection_model->temporary_processing_color);
				updateStickPosition();
			}

			int i = 0;  // Start of the first half in temp
			int j = mid - left + 1;  // Start of the second half in temp
			k = left;  // Start position in the original array to merge back

			// Merge elements back to the original array from temp
			while (i < mid - left + 1 && j < temp.size()) {
				number_of_comparisons++;
				number_of_array_access += 2;
				if (temp[i]->data <= temp[j]->data) {
					sticks[k] = temp[i++];
					number_of_array_access++;
				}
				else {
					sticks[k] = temp[j++];
					number_of_array_access++;
				}

				sound->playSound(SoundType::COMPARE_SFX);
				sticks[k]->stick_view->setFillColor(collection_model->processing_element_color);
				updateStickPosition();  // Immediate update after assignment
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

				k++;
			}

			// Handle remaining elements from both halves
			while (i < mid - left + 1 || j < temp.size()) {
				number_of_array_access++;
				if (i < mid - left + 1) {
					sticks[k] = temp[i++];
				}
				else {
					sticks[k] = temp[j++];
				}

				sound->playSound(SoundType::COMPARE_SFX);
				sticks[k]->stick_view->setFillColor(collection_model->processing_element_color);
				updateStickPosition();  // Immediate update
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

				k++;
			}
		}

		// Out-of-Place Merge Sort function
		void StickCollectionController::mergeSort(int left, int right)
		{
			if (left >= right) return;
			int mid = left + (right - left) / 2;

			mergeSort(left, mid);
			mergeSort(mid + 1, right);
			merge(left, mid, right);
		}

		void StickCollectionController::inPlaceMergeSort(int left, int right) {
			if (left < right) {
				int middle = left + (right - left) / 2;

				// Sort first and second halves
				inPlaceMergeSort(left, middle);
				inPlaceMergeSort( middle + 1, right);

				inPlaceMerge(left, middle, right);
			}
			
		}

		void StickCollectionController::inPlaceMerge(int left, int mid, int right) {
			Sound::SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();

			int start2 = mid + 1;
			if (sticks[mid]->data <= sticks[start2]->data) {
				number_of_array_access += 2;
				number_of_comparisons++;
				return;
			}
			
			while (left <= mid && start2 <= right) {
				number_of_array_access += 2;
				number_of_comparisons++;
				if (sticks[left]->data <= sticks[start2]->data) {
					left++;
				}
				else {
					Stick* swpaData = sticks[start2];
					int index = start2;
					while (index != left) {
						sticks[index] = sticks[index - 1];
						number_of_array_access += 2;
						index--;
					}
					sticks[left] = swpaData;
					number_of_array_access++;
					left++;
					mid++;
					start2++;
					updateStickPosition();
				}
				
				sound->playSound(Sound::SoundType::COMPARE_SFX);
				sticks[left-1]->stick_view->setFillColor(collection_model->processing_element_color);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			
		}

		void StickCollectionController::setCompletedColor() {
			for (int k = 0; k < sticks.size(); k++) {
				if (sort_state == SortState::NOT_SORTING) { break; } // Check if sorting is stoped or completed
				sticks[k]->stick_view->setFillColor(collection_model->element_color);
			}
			SoundService* sound = Global::ServiceLocator::getInstance()->getSoundService();
			for (int i = 0; i < sticks.size(); ++i) {
				if (sort_state == SortState::NOT_SORTING) { break; }  // Check if sorting is stoped or completed
				sound->playSound(SoundType::COMPARE_SFX);
				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);
				std::this_thread::sleep_for(std::chrono::milliseconds(color_delay));
			}
			
		}

		void StickCollectionController::resetSticksColor()
		{
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->setFillColor(collection_model->element_color);
		}

		void StickCollectionController::resetVariables()
		{
			number_of_comparisons = 0;
			number_of_array_access = 0;
		}

		void StickCollectionController::reset()
		{
			current_operation_delay = 0;
			color_delay = 0;
			if (sort_thread.joinable()) sort_thread.join();
			sort_state = SortState::NOT_SORTING;
			shuffleSticks();
			resetSticksColor();
			resetVariables();
		}

		void StickCollectionController::sortElements(SortType sort_type)
		{
			sort_state = SortState::SORTING;
			current_operation_delay = collection_model->operation_delay;
			this->sort_type = sort_type;

			switch (sort_type)
			{
			case Gameplay::Collection::SortType::BUBBLE_SORT:
				sort_thread = std::thread(&StickCollectionController::processBubbleSort, this);
				break;
			case Gameplay::Collection::SortType::INSERTION_SORT:
				sort_thread = std::thread(&StickCollectionController::processInsertionSort, this);
				break;
			case Gameplay::Collection::SortType::SELECTION_SORT:
				sort_thread = std::thread(&StickCollectionController::processSelectionSort, this);
			case Gameplay::Collection::SortType::MERGE_SORT:
				sort_thread = std::thread(&StickCollectionController::processMergeSort, this);
				break;
			case Gameplay::Collection::SortType::QUICK_SORT:
				sort_thread = std::thread(&StickCollectionController::processQuickSort, this);
			case Gameplay::Collection::SortType::RADIX_SORT:
				sort_thread = std::thread(&StickCollectionController::processRadixSort, this);
				break;
			}
		}

		bool StickCollectionController::isCollectionSorted()
		{
			for (int i = 1; i < sticks.size(); i++) if (sticks[i] < sticks[i - 1]) return false;
			return true;
		}

		void StickCollectionController::destroy()
		{
			current_operation_delay = 0;
			if (sort_thread.joinable()) sort_thread.join();

			for (int i = 0; i < sticks.size(); i++) delete(sticks[i]);
			sticks.clear();

			delete (collection_view);
			delete (collection_model);
		}

		SortType StickCollectionController::getSortType() { return sort_type; }

		int StickCollectionController::getNumberOfComparisons() { return number_of_comparisons; }

		int StickCollectionController::getNumberOfArrayAccess() { return number_of_array_access; }

		int StickCollectionController::getNumberOfSticks() { return collection_model->number_of_elements; }

		int StickCollectionController::getDelayMilliseconds() { return current_operation_delay; }

		sf::String StickCollectionController::getTimeComplexity() { return time_complexity; }
	}
}


